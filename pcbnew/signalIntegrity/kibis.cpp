#include "kibis.h"
#include "ibis2kibis.h"
#include <wx/textfile.h>

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

bool KIBIS_MODEL::writeSpiceDriver( wxString* aDest )
{
    double   ton = 20e-9;
    double   toff = 60e-9;
    bool     status = true;

    wxString result;

    result = "\n";
    result += "CCPOMP DIE GND ";
    result << m_C_comp.typ;
    result += "\n";

    result += "BDIEBUFF DIEBUFF GND v=v(DIE)\n";

    result += "VmeasPD GND GND2 0\n";
    result += "VmeasPU POWER POWER2 0\n";
    result += "BKU POWER DIE i=( i(VmeasPD) * v(KD) )\n";
    result += "BKD GND DIE i=( -i(VmeasPU) * v(KU) )\n";

    result += m_GNDClamp.Spice( 1, "DIE", "GND", "GNDClampDiode" );
    result += m_POWERClamp.Spice( 2, "DIE", "POWER", "POWERClampDiode" );
    result += m_pulldown.Spice( 3, "DIEBUFF", "GND2", "Pulldown" );
    result += m_pullup.Spice( 4, "DIEBUFF", "POWER2", "Pullup" );

    *aDest = result;

    return status;
}


// As defined per IBIS, the final value using the ramp is dv_dt / 0.6 + pulldownReference
// That's because the dv_dt value of the ramp is only the 20%-80% edge
// If we have Zu and Zd being the impedance of the pullup and pulldown transistors.
// We add Zu and Zd are knwon and time independent.
// But we add modifiers Ku and Kd, which are time dependent.
// Equation at the beginning of the rising edge is :
// { K_{d"|"{t=0}}  Z_d } over { K_{u"|"{t=0}} Z_u + K_{d"|"{t=0}} Z_d } . ( V_pullupREF - V_PUREF ) + V_PDREF = V_OL
// We have two unknowns, in order to get a second equation, we suppose that both transistor switch synchronously
// Kd = 1 - Ku
//
/*

std::vector< std::pair< double, double > >  KIBIS_PIN::getKuKdRamp( KIBIS_MODEL* aModel, IBIS_WAVEFORM_TYPE aType )
{
    std::vector< std::pair< double, double > > Ku;
    
    std::pair< double, double > point;

    point.first = 0;
    point.second = 0;
    Ku.push_back( point );
    point.first = aModel->m_ramp.m_rising.m_typ.m_dt / 0.6 ; // The ramp is only 20%-80%
    point.second = aModel->m_ramp.m_rising.m_typ.m_dv / 0.6 ; // The ramp is only 20%-80%
    Ku.push_back( point );

    point.second = aModel->m_pulldownReference.typ;
    Ku.push_back( point );

    return Ku;
}*/

//@TODO that function is far from being perfect. Improve it.

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

    double DCtyp = aIn->m_table.m_entries[0].V.typ;
    double DCmin = aIn->m_table.m_entries[0].V.min;
    double DCmax = aIn->m_table.m_entries[0].V.max;

    if( nbPoints == 2 )
    {
        return out;
    }

    out.m_table.m_entries.clear();
    bool kept = false;

    for( int i = 0; i < nbPoints; i++ )
    {
        out.m_table.m_entries.push_back( aIn->m_table.m_entries.at( i ) );
        out.m_table.m_entries.at( i ).V.typ -= DCtyp;
        out.m_table.m_entries.at( i ).V.min -= DCmin;
        out.m_table.m_entries.at( i ).V.max -= DCmax;
    }

    return out;
}

wxString KIBIS_MODEL::generateSquareWave( wxString aNode1, wxString aNode2, double aTon,
                                          double aToff, int aCycles )
{
    wxString simul;

    IbisWaveform risingWF = TrimWaveform( m_risingWaveforms.at( 0 ) );
    IbisWaveform fallingWF = TrimWaveform( m_fallingWaveforms.at( 0 ) );

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
            simul << entry.V.typ;
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
            simul << entry.V.typ;
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

wxString KIBIS_PIN::getKuKdOneWaveform( KIBIS_MODEL* aModel )
{
    double ton = 5e-9;
    double toff = 5e-9;

    wxString simul = "";
    simul += "*THIS IS NOT A VALID SPICE MODEL.\n";
    simul += "*This part is intended to be executed by Kibis internally.\n";
    simul += "*You should not be able to read this.\n\n";
    simul += ".SUBCKT DRIVER POWER GND OUT \n"; // 1: POWER, 2:GND, 3:OUT

    simul += "RPIN 1 OUT ";
    simul << aModel->m_risingWaveforms.at( 0 )->m_R_dut;
    simul += "\n";
    simul += "LPIN 2 1 ";
    simul << aModel->m_risingWaveforms.at( 0 )->m_L_dut;
    simul += "\n";
    simul += "CPIN OUT GND ";
    simul << aModel->m_risingWaveforms.at( 0 )->m_C_dut;
    simul += "\n";

    simul += "\n";
    simul += "CCPOMP 2 GND ";
    simul << aModel->m_C_comp.typ;
    simul += "\n";
    simul += aModel->generateSquareWave( "DIE", "GND", ton, toff, 3 );
    simul += aModel->m_GNDClamp.Spice( 1, "DIE", "GND1", "GNDClampDiode" );
    simul += aModel->m_POWERClamp.Spice( 2, "DIE", "POWER1", "POWERClampDiode" );
    simul += aModel->m_pulldown.Spice( 3, "DIE", "GND2", "Pulldown" );
    simul += aModel->m_pullup.Spice( 4, "DIE", "POWER2", "Pullup" );


    simul += ") \n";
    simul += "\n.ENDS DRIVER\n\n";
    simul += "\n x1 3 0 1 DRIVER \n";
    // In IBIS Currents are positive when entering the component
    // In SPICE, currents are positive when flowing from the first terminal to the second terminal
    // => The first terminal is outside, the second inside
    simul += "VCC 3 0 1.8\n";
    //simul += "Vpin x1.DIE 0 1 \n"
    simul += "Lfixture 1 4 ";
    simul << aModel->m_risingWaveforms.at( 0 )->m_L_fixture;
    simul += "\n";
    simul += "Rfixture 4 5 ";
    simul << aModel->m_risingWaveforms.at( 0 )->m_R_fixture;
    simul += "\n";
    simul += "Cfixture 5 0 ";
    simul << aModel->m_risingWaveforms.at( 0 )->m_C_fixture;
    simul += "\n";
    simul += "Vfixture 5 0 ";
    simul << aModel->m_risingWaveforms.at( 0 )->m_V_fixture;
    simul += "\n";
    simul += "VmeasIout x1.2 x1.DIE 0\n";
    simul += "VmeasPD 0 x1.GND2 0\n";
    simul += "VmeasPU x1.POWER2 3 0\n";
    simul += "VmeasPC x1.POWER1 3 0\n";
    simul += "VmeasGC 0 x1.GND1 0\n";
    simul += "Bku KU 0 v=(i(VmeasIout)+i(VmeasPC)+i(VmeasGC)+i(VmeasPD)/(i(VmeasPD)-i(VmeasPU)))\n";
    simul += "Bkd KD 0 v=(1-v(KU))\n";
    simul += ".tran 0.1n 40n \n";
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


bool KIBIS_PIN::writeSpiceDriver( wxString* aDest, KIBIS_MODEL* aModel )
{
    wxString result;
    wxString tmp;

    result += "\n";
    result = "*Model generated by Kicad using Ibis data. ";
    result += "\n.SUBCKT DRIVER POWER GND OUT \n"; // 1: POWER, 2:GND, 3:OUT
    result += "\n";

    result += "RPIN 1 OUT ";
    result << R_pin.typ;
    result += "\n";
    result += "LPIN DIE 1 ";
    result << L_pin.typ;
    result += "\n";
    result += "CPIN OUT GND ";
    result << C_pin.typ;
    result += "\n";

    aModel->writeSpiceDriver( &tmp );


    std::vector<std::pair<IbisWaveform*, IbisWaveform*>> wfPairs = aModel->waveformPairs();

    if( wfPairs.size() < 1 )
    {
        std::cout << "Model has no waveform pair, using [Ramp] instead, poor accuracy" << std::endl;
    }
    else if( wfPairs.size() == 1 || true )
    {
        std::cout << "Model has only one waveform pair, reduced accuracy" << std::endl;
        getKuKdOneWaveform( aModel );
    }
    else
    {
        if( wfPairs.size() > 2 )
        {
            std::cout << "Model has more than 2 waveform pairs, using the first two." << std::endl;
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

    return true;
}