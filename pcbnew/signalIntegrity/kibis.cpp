#include "kibis.h"
#include "ibis2kibis.h"
#include <wx/textfile.h>

IBIS_CORNER ReverseLogic( IBIS_CORNER aIn )
{
    IBIS_CORNER out = IBIS_CORNER::TYP;
    if( aIn == IBIS_CORNER::MIN )
    {
        out = IBIS_CORNER::MAX;
    }
    else if( aIn == IBIS_CORNER::MAX )
    {
        out = IBIS_CORNER::MIN;
    }
    return out;
}

bool KibisFromFile( wxString aFileName, std::vector<KIBIS_COMPONENT*>* aDest )
{
    IbisParser* parser = new IbisParser();
    IbisFile*   file = new IbisFile();
    parser->m_parrot = true;
    parser->parseFile( wxFileName( aFileName ), file );
    convertKibisAll( parser, aDest );

    return true;
}


KIBIS_PIN* KIBIS_COMPONENT::getPin( wxString aPinNumber )
{
    for( KIBIS_PIN* pin : m_pins )
    {
        if( pin->m_pinNumber == aPinNumber )
        {
            return pin;
        }
    }

    return nullptr;
}

std::vector<std::pair<IbisWaveform*, IbisWaveform*>> KIBIS_MODEL::waveformPairs()
{
    std::vector<std::pair<IbisWaveform*, IbisWaveform*>> pairs;
    IbisWaveform*                                        wf1;
    IbisWaveform*                                        wf2;

    for( int i = 0; i < m_risingWaveforms.size(); i++ )
    {
        for( int j = 0; j < m_fallingWaveforms.size(); j++ )
        {
            wf1 = m_risingWaveforms.at( i );
            wf2 = m_fallingWaveforms.at( j );

            if( wf1->m_R_fixture == wf2->m_R_fixture && wf1->m_L_fixture == wf2->m_L_fixture
                && wf1->m_C_fixture == wf2->m_C_fixture && wf1->m_V_fixture == wf2->m_V_fixture
                && wf1->m_V_fixture_min == wf2->m_V_fixture_min
                && wf1->m_V_fixture_max == wf2->m_V_fixture_max )
            {
                std::pair<IbisWaveform*, IbisWaveform*> p;
                p.first = wf1;
                p.second = wf2;
                pairs.push_back( p );

                std::cout << "WAVEFORM PAIR :" << std::endl;
                std::cout << wf1->m_R_fixture << std::endl;
                std::cout << wf1->m_L_fixture << std::endl;
                std::cout << wf1->m_C_fixture << std::endl;
                std::cout << wf1->m_V_fixture << std::endl;
            }
        }
    }

    return pairs;
}

wxString KIBIS_MODEL::SpiceDie( IBIS_CORNER aSupply, IBIS_CORNER aSpeed )
{
    wxString result;

    result = "\n";
    result += "CCPOMP DIE GND ";
    result << m_C_comp.value[ReverseLogic( aSpeed )];
    result += "\n";
    result += m_GNDClamp.Spice( 1, "DIE", "GND", "GNDClampDiode", aSupply );
    result += m_POWERClamp.Spice( 2, "DIE", "POWER", "POWERClampDiode", aSupply );
    result += m_pulldown.Spice( 3, "DIEBUFF", "GND2", "Pulldown", aSupply );
    result += m_pullup.Spice( 4, "POWER2", "DIEBUFF", "Pullup", aSupply );
    result += "BDIEBUFF DIEBUFF GND v=v(DIE)\n";

    result += "VmeasPD GND GND2 0\n";
    result += "VmeasPU POWER POWER2 0\n";
    result += "BKU POWER DIE i=( i(VmeasPD) * v(KD) )\n";
    result += "BKD GND DIE i=( -i(VmeasPU) * v(KU) )\n";


    return result;
}

IbisWaveform TrimWaveform( IbisWaveform* aIn )
{
    IbisWaveform out;

    out = *aIn; //@TODO : deepcopy ?
    int nbPoints = out.m_table.m_entries.size();

    if( nbPoints < 2 )
    {
        std::cerr << "ERROR : waveform has less than two points" << std::endl;
        return out;
    }

    double DCtyp = aIn->m_table.m_entries[0].V.value[IBIS_CORNER::TYP];
    double DCmin = aIn->m_table.m_entries[0].V.value[IBIS_CORNER::MIN];
    double DCmax = aIn->m_table.m_entries[0].V.value[IBIS_CORNER::MAX];

    if( nbPoints == 2 )
    {
        return out;
    }

    out.m_table.m_entries.clear();
    bool kept = false;

    for( int i = 0; i < nbPoints; i++ )
    {
        out.m_table.m_entries.push_back( aIn->m_table.m_entries.at( i ) );
        out.m_table.m_entries.at( i ).V.value[IBIS_CORNER::TYP] -= DCtyp;
        out.m_table.m_entries.at( i ).V.value[IBIS_CORNER::MIN] -= DCmin;
        out.m_table.m_entries.at( i ).V.value[IBIS_CORNER::MAX] -= DCmax;
    }

    return out;
}

bool KIBIS_MODEL::HasPulldown()
{
    return m_pulldown.m_entries.size() > 0;
}
bool KIBIS_MODEL::HasPullup()
{
    return m_pullup.m_entries.size() > 0;
}
bool KIBIS_MODEL::HasGNDClamp()
{
    return m_GNDClamp.m_entries.size() > 0;
}
bool KIBIS_MODEL::HasPOWERClamp()
{
    return m_POWERClamp.m_entries.size() > 0;
}

wxString KIBIS_MODEL::generateSquareWave( wxString aNode1, wxString aNode2,
                                          KIBIS_WAVEFORM_RECTANGULAR*             aWave,
                                          std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                          IBIS_CORNER                             aSupply )
{
    wxString simul;

    IbisWaveform risingWF = TrimWaveform( aPair.first );
    IbisWaveform fallingWF = TrimWaveform( aPair.second );
    std::cout << "Times : " << aWave->m_ton << "  " << aWave->m_toff << std::endl;

    if( aWave->m_ton < risingWF.m_table.m_entries.at( risingWF.m_table.m_entries.size() - 1 ).t )
    {
        std::cerr << "WARNING: rising edge is longer than on time." << std::endl;
    }

    if( aWave->m_toff < fallingWF.m_table.m_entries.at( fallingWF.m_table.m_entries.size() - 1 ).t )
    {
        std::cerr << "WARNING: falling edge is longer than on time." << std::endl;
    }

    for( int i = 0; i < aWave->m_cycles; i++ )
    {
        simul += "Vrise";
        simul << i;
        simul << " rise";
        simul << i;
        simul << " ";
        simul << aNode2;
        simul += " pwl ( ";

        for( auto entry : risingWF.m_table.m_entries )
        {
            simul << entry.t + i * ( aWave->m_ton + aWave->m_toff );
            simul += " ";
            simul << entry.V.value[aSupply];
            simul += " ";
        }
        simul += ")\n";
    }

    for( int i = 0; i < aWave->m_cycles; i++ )
    {
        simul += "Vfall";
        simul << i;
        simul << " fall";
        simul << i;
        simul << " ";
        simul << aNode2;
        simul += " pwl ( ";

        for( auto entry : fallingWF.m_table.m_entries )
        {
            simul << entry.t + i * ( aWave->m_ton + aWave->m_toff ) + aWave->m_ton;
            simul += " ";
            simul << entry.V.value[aSupply];
            simul += " ";
        }
        simul += ")\n";
    }

    simul += "bin ";
    simul += aNode1;
    simul += " ";
    simul += aNode2;
    simul += " v=(";

    for( int i = 0; i < aWave->m_cycles; i++ )
    {
        simul += " v( rise";
        simul << i;
        simul += " ) + v( fall";
        simul << i;
        simul += ") ";
        simul += "+";
    }
    simul << aPair.first->m_table.m_entries.at( 0 ).V.value[aSupply]; // Add DC bias

    simul += ")\n";
    return simul;
}

wxString KIBIS_PIN::addDie( KIBIS_MODEL* aModel, IBIS_CORNER aSupply, int aIndex )
{
    wxString simul;

    wxString GC_GND = "GC_GND";
    wxString PC_PWR = "PC_PWR";
    wxString PU_PWR = "PU_PWR";
    wxString PD_GND = "PD_GND";
    wxString DIE = "DIE";

    GC_GND << aIndex;
    PC_PWR << aIndex;
    PU_PWR << aIndex;
    PD_GND << aIndex;
    DIE << aIndex;


    wxString GC = "GC";
    wxString PC = "PC";
    wxString PU = "PU";
    wxString PD = "PD";

    GC << aIndex;
    PC << aIndex;
    PU << aIndex;
    PD << aIndex;

    if( aModel->HasGNDClamp() )
    {
        simul += aModel->m_GNDClamp.Spice( aIndex * 4 + 1, DIE, GC_GND, GC, aSupply );
    }
    if( aModel->HasPOWERClamp() )
    {
        simul += aModel->m_POWERClamp.Spice( aIndex * 4 + 2, DIE, PC_PWR, PC, aSupply );
    }
    if( aModel->HasPulldown() )
    {
        simul += aModel->m_pulldown.Spice( aIndex * 4 + 3, DIE, PD_GND, PD, aSupply );
    }
    if( aModel->HasPullup() )
    {
        simul += aModel->m_pullup.Spice( aIndex * 4 + 4, PU_PWR, DIE, PU, aSupply );
    }

    return simul;
}

void KIBIS_PIN::getKuKdFromFile( wxString* aSimul )
{
    // @TODO
    // that's not the best way to do, but ¯\_(ツ)_/¯
    wxTextFile file( "temp_input.spice" );
    if( file.Exists() )
    {
        file.Clear();
        file.Write();
    }
    else
    {
        file.Create();
    }
    file.AddLine( *aSimul );
    file.Write();

    wxTextFile file2( "temp_output.spice" );
    if( file2.Exists() )
    {
        file2.Clear();
        file2.Write();
    }
    system( "ngspice temp_input.spice" );


    std::ifstream KuKdfile;
    KuKdfile.open( "temp_output.spice" );

    std::vector<double> ku, kd, t;
    if( KuKdfile )
    {
        std::string line;
        for( int i = 0; i < 11; i++ ) // number of line in the ngspice output header
        {
            std::getline( KuKdfile, line );
        }
        int    i = 0;
        double t_v, ku_v, kd_v;
        while( KuKdfile )
        {
            std::getline( KuKdfile, line );

            if( line.empty() )
            {
                continue;
            }
            switch( i )
            {
            case 0:
                line = line.substr( line.find_first_of( "\t" ) + 1 );
                t_v = std::stod( line );
                break;
            case 1: ku_v = std::stod( line ); break;
            case 2:
                kd_v = std::stod( line );
                ku.push_back( ku_v );
                kd.push_back( kd_v );
                t.push_back( t_v );
                break;
            default: std::cerr << "ERROR : i should be between 0 and 2." << std::endl;
            }
            i = ( i + 1 ) % 3;
        }
        std::getline( KuKdfile, line );
    }
    else
    {
        std::cerr << "ERROR : I asked for file creation, but I cannot find it" << std::endl;
    }
    // @TODO : this is the end of the dirty code

    m_Ku = ku;
    m_Kd = kd;
    m_t = t;
}

wxString KIBIS_PIN::KuKdDriver( KIBIS_MODEL* aModel, std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType,
                                IBIS_CORNER aSupply, IBIS_CORNER aSpeed, int index )
{
    wxString simul = "";

    simul += "*THIS IS NOT A VALID SPICE MODEL.\n";
    simul += "*This part is intended to be executed by Kibis internally.\n";
    simul += "*You should not be able to read this.\n\n";

    simul += ".SUBCKT DRIVER";
    simul << index;
    simul += " POWER GND OUT \n"; // 1: POWER, 2:GND, 3:OUT

    if( ( aPair.first->m_R_dut == 0 ) && ( aPair.first->m_L_dut == 0 )
                && ( aPair.first->m_C_dut == 0 )
        || true )
    {
        simul += "Vdummy 2 OUT 0\n";
    }
    {
        std::cout << "R:" << aPair.first->m_R_dut << std::endl;
        std::cout << "L:" << aPair.first->m_L_dut << std::endl;
        std::cout << "C:" << aPair.first->m_C_dut << std::endl;
        /*
        simul += "RPIN 1 OUT ";
        simul << aPair.first->m_R_dut;
        simul += "\n";
        simul += "LPIN 2 1 ";
        simul << aPair.first->m_L_dut;
        simul += "\n";
        simul += "CPIN OUT GND ";
        simul << aPair.first->m_C_dut;
        simul += "\n";
        */
        std::cerr
                << "WARNING : I can't yet use DUT values. https://ibis.org/summits/nov16a/chen.pdf"
                << std::endl;
    }

    simul += "\n";
    simul += "CCPOMP 2 GND ";
    simul << aModel->m_C_comp.value[aSpeed]; //@TODO: Check the corner ?
    simul += "\n";
    switch( aWaveType )
    {
    case KIBIS_WAVEFORM_TYPE::RECTANGULAR:
        simul += aModel->generateSquareWave(
                "DIE0", "GND", static_cast<KIBIS_WAVEFORM_RECTANGULAR*>( aWave ), aPair, aSupply );
        break;
    case KIBIS_WAVEFORM_TYPE::NONE:
    default: std::cout << "NO WAVEFORM" << std::endl; break;
    }
    simul += addDie( aModel, aSupply, 0 );

    simul += "\n.ENDS DRIVER\n\n";
    return simul;
}

wxString KIBIS_PIN::getKuKdOneWaveform( KIBIS_MODEL*                            aModel,
                                        std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                        KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType,
                                        IBIS_CORNER aSupply, IBIS_CORNER aSpeed )
{
    wxString simul = "";

    if( aWaveType == KIBIS_WAVEFORM_TYPE::NONE )
    {
        //@TODO , there could be some current flowing through pullup / pulldown transistors, even when off
        std::vector<double> ku, kd, t;
        ku.push_back( 0 );
        kd.push_back( 0 );
        t.push_back( 0 );
        m_Ku = ku;
        m_Kd = kd;
        m_t = t;
    }
    else
    {
        simul += KuKdDriver( aModel, aPair, aWave, aWaveType, aSupply, aSpeed, 0 );
        simul += "\n x1 3 0 1 DRIVER0 \n";

        simul += "VCC 3 0 ";
        simul << aModel->m_voltageRange.value[aSupply];
        simul += "\n";
        //simul += "Vpin x1.DIE 0 1 \n"
        simul += "Lfixture 1 4 ";
        simul << aPair.first->m_L_fixture;
        simul += "\n";
        simul += "Rfixture 4 5 ";
        simul << aPair.first->m_R_fixture;
        simul += "\n";
        simul += "Cfixture 4 0 ";
        simul << aPair.first->m_C_fixture;
        simul += "\n";
        simul += "Vfixture 5 0 ";
        simul << aPair.first->m_V_fixture;
        simul += "\n";
        simul += "VmeasIout x1.2 x1.DIE0 0\n";
        simul += "VmeasPD 0 x1.PD_GND0 0\n";
        simul += "VmeasPU x1.PU_PWR0 3 0\n";
        simul += "VmeasPC x1.PC_PWR0 3 0\n";
        simul += "VmeasGC 0 x1.GC_PWR0 0\n";

        if( aModel->HasPullup() && aModel->HasPulldown() )
        {
            std::cout << "Model has only one waveform pair, reduced accuracy" << std::endl;
            simul += "Bku KU 0 v=( (i(VmeasIout)+i(VmeasPC)+i(VmeasGC)+i(VmeasPD) "
                     ")/(i(VmeasPD)-i(VmeasPU)))\n";
            simul += "Bkd KD 0 v=(1-v(KU))\n";
        }

        else if( !aModel->HasPullup() && aModel->HasPulldown() )
        {
            simul += "Bku KD 0 v=( ( i(VmeasIout)+i(VmeasPC)+i(VmeasGC) )/(i(VmeasPD)))\n";
            simul += "Bkd KU 0 v=0\n";
        }

        else if( aModel->HasPullup() && !aModel->HasPulldown() )
        {
            simul += "Bku KU 0 v=( ( i(VmeasIout)+i(VmeasPC)+i(VmeasGC) )/(i(VmeasPU)))\n";
            simul += "Bkd KD 0 v=0\n";
        }
        else
        {
            std::cout << "ERROR: Driver needs at least a pullup or a pulldown" << std::endl;
        }


        simul += ".tran 0.1n 100n \n";
        simul += ".option xmu=0.49  \n";
        //simul += ".dc Vpin -5 5 0.1\n";
        simul += ".control run \n";
        simul += "set filetype=ascii\n";
        simul += "run \n";
        simul += "plot v(x1.DIE) i(VmeasIout) i(VmeasPD) i(VmeasPU) i(VmeasPC) i(VmeasGC) "
                 "v(x1.POWER2)\n";
        simul += "plot v(KU) v(KD)\n";
        simul += "write temp_output.spice v(KU) v(KD)\n"; // @TODO we might want to remove this...
        //simul += "quit\n";
        simul += ".endc \n";
        simul += ".end \n";

        getKuKdFromFile( &simul );
    }
    return simul;
}

void KIBIS_PIN::getKuKdNoWaveform( KIBIS_MODEL* aModel, KIBIS_WAVEFORM* aWave,
                                   KIBIS_WAVEFORM_TYPE aWaveType, IBIS_CORNER aSupply )
{
    std::vector<double> ku, kd, t;

    switch( aWaveType )
    {
    case KIBIS_WAVEFORM_TYPE::RECTANGULAR:
    {
        KIBIS_WAVEFORM_RECTANGULAR* rectWave = static_cast<KIBIS_WAVEFORM_RECTANGULAR*>( aWave );
        for( int i = 0; i < 10; i++ )
        {
            ku.push_back( 0 );
            kd.push_back( 1 );
            t.push_back( ( rectWave->m_ton + rectWave->m_toff ) * i );
            ku.push_back( 1 );
            kd.push_back( 0 );
            t.push_back( ( rectWave->m_ton + rectWave->m_toff ) * i
                         + aModel->m_ramp.m_rising.value[aSupply].m_dt
                                   / 0.6 ); // 0.6 because ibis only gives 20%-80% time
            ku.push_back( 1 );
            kd.push_back( 0 );
            t.push_back( ( rectWave->m_ton + rectWave->m_toff ) * i + rectWave->m_toff );
            ku.push_back( 0 );
            kd.push_back( 1 );
            t.push_back( ( rectWave->m_ton + rectWave->m_toff ) * i + rectWave->m_toff
                         + aModel->m_ramp.m_falling.value[aSupply].m_dt / 0.6 );
        }
        break;
    }
    case KIBIS_WAVEFORM_TYPE::NONE:
    default:
        ku.push_back( 0 );
        kd.push_back( 0 );
        t.push_back( 0 );
    }
    m_Ku = ku;
    m_Kd = kd;
    m_t = t;
}


wxString KIBIS_PIN::getKuKdTwoWaveforms( KIBIS_MODEL*                            aModel,
                                         std::pair<IbisWaveform*, IbisWaveform*> aPair1,
                                         std::pair<IbisWaveform*, IbisWaveform*> aPair2,
                                         KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType,
                                         IBIS_CORNER aSupply, IBIS_CORNER aSpeed )
{
    wxString simul = "";

    if( aWaveType == KIBIS_WAVEFORM_TYPE::NONE )
    {
        //@TODO , there could be some current flowing through pullup / pulldown transistors, even when off
        std::vector<double> ku, kd, t;
        ku.push_back( 0 );
        kd.push_back( 0 );
        t.push_back( 0 );
        m_Ku = ku;
        m_Kd = kd;
        m_t = t;
    }
    else
    {
        simul += KuKdDriver( aModel, aPair1, aWave, aWaveType, aSupply, aSpeed, 0 );
        simul += KuKdDriver( aModel, aPair2, aWave, aWaveType, aSupply, aSpeed, 1 );
        simul += "\n x1 3 0 1 DRIVER0 \n";

        simul += "VCC 3 0 ";
        simul << aModel->m_voltageRange.value[aSupply];
        simul += "\n";
        //simul += "Vpin x1.DIE 0 1 \n"
        simul += "Lfixture0 1 4 ";
        simul << aPair1.first->m_L_fixture;
        simul += "\n";
        simul += "Rfixture0 4 5 ";
        simul << aPair1.first->m_R_fixture;
        simul += "\n";
        simul += "Cfixture0 4 0 ";
        simul << aPair1.first->m_C_fixture;
        simul += "\n";
        simul += "Vfixture0 5 0 ";
        simul << aPair1.first->m_V_fixture;
        simul += "\n";
        simul += "VmeasIout0 x1.2 x1.DIE0 0\n";
        simul += "VmeasPD0 0 x1.PD_GND0 0\n";
        simul += "VmeasPU0 x1.PU_PWR0 3 0\n";
        simul += "VmeasPC0 x1.PC_PWR0 3 0\n";
        simul += "VmeasGC0 0 x1.GC_PWR0 0\n";


        simul += "\n x2 3 0 7 DRIVER1 \n";
        //simul += "Vpin x1.DIE 0 1 \n"
        simul += "Lfixture1 7 8 ";
        simul << aPair2.first->m_L_fixture;
        simul += "\n";
        simul += "Rfixture1 8 9 ";
        simul << aPair2.first->m_R_fixture;
        simul += "\n";
        simul += "Cfixture1 8 0 ";
        simul << aPair2.first->m_C_fixture;
        simul += "\n";
        simul += "Vfixture1 9 0 ";
        simul << aPair2.first->m_V_fixture;
        simul += "\n";
        simul += "VmeasIout1 x2.2 x2.DIE0 0\n";
        simul += "VmeasPD1 0 x2.PD_GND0 0\n";
        simul += "VmeasPU1 x2.PU_PWR0 3 0\n";
        simul += "VmeasPC1 x2.PC_PWR0 3 0\n";
        simul += "VmeasGC1 0 x2.GC_PWR0 0\n";

        if( aModel->HasPullup() && aModel->HasPulldown() )
        {
            simul +=
                    "Bku KU 0 v=(  ( i(VmeasPD1) * ( i(VmeasIout0) + i(VmeasPC0) + i(VmeasGC0) ) - "
                    "i(VmeasPD0) * ( i(VmeasIout1) + i(VmeasPC1) + i(VmeasGC1) )  )/ ( i(VmeasPU1) "
                    "* "
                    "i(VmeasPD0) - i(VmeasPU0) * i(VmeasPD1)  ) )\n";
            simul +=
                    "Bkd KD 0 v=(  ( i(VmeasPU1) * ( i(VmeasIout0) + i(VmeasPC0) + i(VmeasGC0) ) - "
                    "i(VmeasPU0) * ( i(VmeasIout1) + i(VmeasPC1) + i(VmeasGC1) )  )/ ( i(VmeasPD1) "
                    "* "
                    "i(VmeasPU0) - i(VmeasPD0) * i(VmeasPU1)  ) )\n";
            //simul += "Bkd KD 0 v=(1-v(KU))\n";
        }

        else if( !aModel->HasPullup() && aModel->HasPulldown() )
        {
            std::cout << " I have two waveforms, but only one transistor. More equations than "
                         "unknowns."
                      << std::endl;
            simul += "Bku KD 0 v=( ( i(VmeasIout0)+i(VmeasPC0)+i(VmeasGC0) )/(i(VmeasPD0)))\n";
            simul += "Bkd KU 0 v=0\n";
        }

        else if( aModel->HasPullup() && !aModel->HasPulldown() )
        {
            std::cout << " I have two waveforms, but only one transistor. More equations than "
                         "unknowns."
                      << std::endl;
            simul += "Bku KU 0 v=( ( i(VmeasIout)+i(VmeasPC)+i(VmeasGC) )/(i(VmeasPU)))\n";
            simul += "Bkd KD 0 v=0\n";
        }
        else
        {
            std::cout << "ERROR: Driver needs at least a pullup or a pulldown" << std::endl;
        }


        simul += ".tran 0.1n 100n \n";
        simul += ".option xmu=0.49  \n";
        //simul += ".dc Vpin -5 5 0.1\n";
        simul += ".control run \n";
        simul += "set filetype=ascii\n";
        simul += "run \n";
        simul += "plot v(KU) v(KD)\n";
        simul += "write temp_output.spice v(KU) v(KD)\n"; // @TODO we might want to remove this...
        //simul += "quit\n";
        simul += ".endc \n";
        simul += ".end \n";

        getKuKdFromFile( &simul );
    }
    return simul;
}

bool KIBIS_PIN::writeSpiceDriver( wxString* aDest, wxString aName, KIBIS_MODEL* aModel,
                                  IBIS_CORNER aSupply, IBIS_CORNER aSpeed, KIBIS_ACCURACY aAccuracy,
                                  KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType )
{
    bool status = true;

    switch( aModel->m_type )
    {
    case IBIS_MODEL_TYPE::OUTPUT:
    case IBIS_MODEL_TYPE::IO:
    case IBIS_MODEL_TYPE::THREE_STATE:
    case IBIS_MODEL_TYPE::OPEN_DRAIN:
    case IBIS_MODEL_TYPE::IO_OPEN_DRAIN:
    case IBIS_MODEL_TYPE::OPEN_SINK:
    case IBIS_MODEL_TYPE::IO_OPEN_SINK:
    case IBIS_MODEL_TYPE::OPEN_SOURCE:
    case IBIS_MODEL_TYPE::IO_OPEN_SOURCE:
    case IBIS_MODEL_TYPE::OUTPUT_ECL:
    case IBIS_MODEL_TYPE::IO_ECL:
    case IBIS_MODEL_TYPE::THREE_STATE_ECL:
    {
        wxString result;
        wxString tmp;

        result += "\n";
        result = "*Driver model generated by Kicad using Ibis data. ";
        result += "\n.SUBCKT ";
        result += aName;
        result += " POWER GND OUT \n"; // 1: POWER, 2:GND, 3:OUT
        result += "\n";

        result += "RPIN 1 OUT ";
        result << R_pin.value[ReverseLogic( aSpeed )];
        result += "\n";
        result += "LPIN DIE 1 ";
        result << L_pin.value[ReverseLogic( aSpeed )];
        result += "\n";
        result += "CPIN OUT GND ";
        result << C_pin.value[ReverseLogic( aSpeed )];
        result += "\n";



        std::vector<std::pair<IbisWaveform*, IbisWaveform*>> wfPairs = aModel->waveformPairs();



        if( wfPairs.size() < 1 || aAccuracy <= KIBIS_ACCURACY::LEVEL_0 )
        {
            if( aAccuracy > KIBIS_ACCURACY::LEVEL_0 )
            {
                std::cout << "Model has no waveform pair, using [Ramp] instead, poor accuracy"
                          << std::endl;
            }
            getKuKdNoWaveform( aModel, aWave, aWaveType, aSupply );
        }
        else if( wfPairs.size() == 1 || aAccuracy <= KIBIS_ACCURACY::LEVEL_1 )
        {
            getKuKdOneWaveform( aModel, wfPairs.at( 0 ), aWave, aWaveType, aSupply, aSpeed );
        }
        else
        {
            if( wfPairs.size() > 2 || aAccuracy <= KIBIS_ACCURACY::LEVEL_2 )
            {
                std::cout << "Model has more than 2 waveform pairs, using the first two."
                          << std::endl;
            }
            getKuKdTwoWaveforms( aModel, wfPairs.at( 0 ), wfPairs.at( 1 ), aWave, aWaveType,
                                 aSupply, aSpeed );
        }

        result += "Vku KU GND pwl ( ";
        for( int i = 0; i < m_t.size(); i++ )
        {
            result << m_t.at( i );
            result += " ";
            result << m_Ku.at( i );
            result += " ";
        }
        result += ") \n";


        result += "Vkd KD GND pwl ( ";
        for( int i = 0; i < m_t.size(); i++ )
        {
            result << m_t.at( i );
            result += " ";
            result << m_Kd.at( i );
            result += " ";
        }

        result += ") \n";

        result += aModel->SpiceDie( aSupply, aSpeed );

        result += "\n.ENDS DRIVER\n\n";

        *aDest = result;
        break;
    }
    default:
        std::cout << "ERROR : Can't define a driver for that model type." << std::endl;
        status = false;
    }

    return status;
}


bool KIBIS_PIN::writeSpiceDevice( wxString* aDest, wxString aName, KIBIS_MODEL* aModel,
                                  IBIS_CORNER aSupply, IBIS_CORNER aSpeed )
{
    bool status = true;

    switch( aModel->m_type )
    {
    case IBIS_MODEL_TYPE::INPUT:
    case IBIS_MODEL_TYPE::IO:
    case IBIS_MODEL_TYPE::IO_OPEN_DRAIN:
    case IBIS_MODEL_TYPE::IO_OPEN_SINK:
    case IBIS_MODEL_TYPE::IO_OPEN_SOURCE:
    case IBIS_MODEL_TYPE::IO_ECL:
    {
        wxString result;
        wxString tmp;

        result += "\n";
        result = "*Device model generated by Kicad using Ibis data. ";
        result += "\n.SUBCKT ";
        result += aName;
        result += " POWER GND OUT \n"; // 1: POWER, 2:GND, 3:OUT
        result += "\n";

        result += "RPIN 1 OUT ";
        result << R_pin.value[ReverseLogic( aSpeed )];
        result += "\n";
        result += "LPIN DIE 1 ";
        result << L_pin.value[ReverseLogic( aSpeed )];
        result += "\n";
        result += "CPIN OUT GND ";
        result << C_pin.value[ReverseLogic( aSpeed )];
        result += "\n";


        result += "Vku KU GND pwl ( 0 0 )\n";
        result += "Vkd KD GND pwl ( 0 0 )\n";

        result += aModel->SpiceDie( aSupply, aSpeed );

        result += "\n.ENDS DRIVER\n\n";

        *aDest = result;
        break;
    }
    default:
        std::cout << "ERROR : Can't define a driver for that model type." << std::endl;
        status = false;
    }

    return status;
}