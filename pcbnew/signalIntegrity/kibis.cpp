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
            }
        }
    }

    return pairs;
}

bool KIBIS_MODEL::writeSpiceDriver( wxString* aDest, IBIS_CORNER aSpeed )
{
    bool     status = true;


    wxString result;

    result = "\n";
    result += "CCPOMP DIE GND ";
    result << m_C_comp.value[ReverseLogic( aSpeed )];
    result += "\n";

    switch( m_type )
    {
    case IBIS_MODEL_TYPE::THREE_STATE:
    case IBIS_MODEL_TYPE::OUTPUT:
        result += "BDIEBUFF DIEBUFF GND v=v(DIE)\n";

        result += "VmeasPD GND GND2 0\n";
        result += "VmeasPU POWER POWER2 0\n";
        result += "BKU POWER DIE i=( i(VmeasPD) * v(KD) )\n";
        result += "BKD GND DIE i=( -i(VmeasPU) * v(KU) )\n";

        result += m_GNDClamp.Spice( 1, "DIE", "GND", "GNDClampDiode", ReverseLogic( aSpeed ) );
        result +=
                m_POWERClamp.Spice( 2, "DIE", "POWER", "POWERClampDiode", ReverseLogic( aSpeed ) );
        result += m_pulldown.Spice( 3, "DIEBUFF", "GND2", "Pulldown", aSpeed );
        result += m_pullup.Spice( 4, "DIEBUFF", "POWER2", "Pullup", aSpeed );

        *aDest = result;
        break;
    default:
        std::cerr << "ERROR: cannot generate output for that model type: ";
        std::cerr << m_name << std::endl;
    }


    return status;
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
    return true;
}
bool KIBIS_MODEL::HasPullup()
{
    return true;
}

wxString KIBIS_MODEL::generateSquareWave( wxString aNode1, wxString aNode2, double aTon,
                                          double aToff, int aCycles,
                                          std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                          IBIS_CORNER                             aSupply )
{
    wxString simul;

    IbisWaveform risingWF = TrimWaveform( aPair.first );
    IbisWaveform fallingWF = TrimWaveform( aPair.second );
    std::cout << "Times : " << aTon << "  " << aToff << std::endl;

    if( aTon < risingWF.m_table.m_entries.at( risingWF.m_table.m_entries.size() - 1 ).t )
    {
        std::cerr << "WARNING: rising edge is longer than on time." << std::endl;
    }

    if( aToff < fallingWF.m_table.m_entries.at( fallingWF.m_table.m_entries.size() - 1 ).t )
    {
        std::cerr << "WARNING: falling edge is longer than on time." << std::endl;
    }

    for( int i = 0; i < aCycles; i++ )
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
            simul << entry.t + i * ( aTon + aToff );
            simul += " ";
            simul << entry.V.value[aSupply];
            simul += " ";
        }
        simul += ")\n";
    }

    for( int i = 0; i < aCycles; i++ )
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
            simul << entry.t + i * ( aTon + aToff ) + aTon;
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

    for( int i = 0; i < aCycles; i++ )
    {
        simul += " v( rise";
        simul << i;
        simul += " ) + v( fall";
        simul << i;
        simul += ") ";
        if( i < aCycles - 1 )
        {
            simul += "+";
        }
    }

    simul += ")\n";
    return simul;
}

wxString KIBIS_PIN::getKuKdOneWaveform( KIBIS_MODEL*                            aModel,
                                        std::pair<IbisWaveform*, IbisWaveform*> aPair, double aTon,
                                        double aToff, IBIS_CORNER aSupply, IBIS_CORNER aSpeed )
{

    wxString simul = "";
    simul += "*THIS IS NOT A VALID SPICE MODEL.\n";
    simul += "*This part is intended to be executed by Kibis internally.\n";
    simul += "*You should not be able to read this.\n\n";
    simul += ".SUBCKT DRIVER POWER GND OUT \n"; // 1: POWER, 2:GND, 3:OUT

    simul += "RPIN 1 OUT ";
    simul << aPair.first->m_R_dut;
    simul += "\n";
    simul += "LPIN 2 1 ";
    simul << aPair.first->m_L_dut;
    simul += "\n";
    simul += "CPIN OUT GND ";
    simul << aPair.first->m_C_dut;
    simul += "\n";

    simul += "\n";
    simul += "CCPOMP 2 GND ";
    simul << aModel->m_C_comp.value[IBIS_CORNER::TYP]; //@TODO: Check that ?
    simul += "\n";
    simul += aModel->generateSquareWave( "DIE", "GND", aTon, aToff, 10, aPair, aSupply );
    simul += aModel->m_GNDClamp.Spice( 1, "DIE", "GND1", "GNDClampDiode", ReverseLogic( aSpeed ) );
    simul += aModel->m_POWERClamp.Spice( 2, "DIE", "POWER1", "POWERClampDiode",
                                         ReverseLogic( aSpeed ) );

    if( aModel->HasPulldown() )
    {
        simul += aModel->m_pulldown.Spice( 3, "DIE", "GND2", "Pulldown", aSpeed );
    }
    if( aModel->HasPullup() )
    {
        simul += aModel->m_pullup.Spice( 4, "DIE", "POWER2", "Pullup", aSpeed );
    }


    simul += "\n.ENDS DRIVER\n\n";
    simul += "\n x1 3 0 1 DRIVER \n";
    // In IBIS Currents are positive when entering the component
    // In SPICE, currents are positive when flowing from the first terminal to the second terminal
    // => The first terminal is outside, the second inside
    simul += "VCC 3 0 1.8\n";
    //simul += "Vpin x1.DIE 0 1 \n"
    simul += "Lfixture 1 4 ";
    simul << aPair.first->m_L_fixture;
    simul += "\n";
    simul += "Rfixture 4 5 ";
    simul << aPair.first->m_R_fixture;
    simul += "\n";
    simul += "Cfixture 5 0 ";
    simul << aPair.first->m_C_fixture;
    simul += "\n";
    simul += "Vfixture 5 0 ";
    simul << aPair.first->m_V_fixture;
    simul += "\n";
    simul += "VmeasIout x1.2 x1.DIE 0\n";
    simul += "VmeasPD 0 x1.GND2 0\n";
    simul += "VmeasPU x1.POWER2 3 0\n";
    simul += "VmeasPC x1.POWER1 3 0\n";
    simul += "VmeasGC 0 x1.GND1 0\n";

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
        std::cout << "ERROR: Driver needs at least a pullup or a pulldown";
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
    simul += "plot v(x1.rise0) v(x1.fall0) v(x1.rise1) v(x1.fall1) v(x1.rise2) v(x1.fall2) \n";
    simul += "write temp_output.spice v(KU) v(KD)\n"; // @TODO we might want to remove this...
    simul += "quit\n";
    simul += ".endc \n";
    simul += ".end \n";

    // @TODO
    // Seriously, that's not the best way to do, but ¯\_(ツ)_/¯
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
    file.AddLine( simul );
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

    return simul;
}


bool KIBIS_PIN::writeSpiceDriver( wxString* aDest, wxString aName, KIBIS_MODEL* aModel,
                                  IBIS_CORNER aSupply, IBIS_CORNER aSpeed )
{
    double ton = 12e-9;
    double toff = 12e-9;

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


        aModel->writeSpiceDriver( &tmp, aSpeed );

        if( wfPairs.size() < 1 )
        {
            std::cout << "Model has no waveform pair, using [Ramp] instead, poor accuracy"
                      << std::endl;
        }
        else if( wfPairs.size() == 1 || true )
        {
            getKuKdOneWaveform( aModel, wfPairs.at( 0 ), ton, toff, aSupply, aSpeed );
        }
        else
        {
            if( wfPairs.size() > 2 )
            {
                std::cout << "Model has more than 2 waveform pairs, using the first two."
                          << std::endl;
            }
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


        result += tmp;
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