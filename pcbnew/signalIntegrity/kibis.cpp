#include "kibis.h"
#include "ibis2kibis.h"

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

    result += m_GNDClamp.Spice( 1, "GND", "DIE", "GNDClampDiode" );
    result += m_POWERClamp.Spice( 2, "POWER", "DIE", "POWERClampDiode" );
    result += m_pulldown.Spice( 3, "GND", "DIE", "Pulldown" );
    result += m_pullup.Spice( 4, "POWER", "DIE", "Pullup" );


    result += "vin DIE GND pwl ( ";

    std::vector<std::pair<IbisWaveform*, IbisWaveform*>> wfPairs = waveformPairs();

    if( wfPairs.size() < 1 )
    {
        std::cout << "Model has no waveform pair, using [Ramp] instead, poor accuracy" << std::endl;
    }
    else if( wfPairs.size() == 1 )
    {
    }
    else
    {
        if( wfPairs.size() > 2 )
        {
            std::cout << "Model has more than 2 waveform pairs, using the first two." << std::endl;
        }
    }

    double lastT;
    for( auto entry : m_risingWaveforms.at( 0 )->m_table.m_entries )
    {
        lastT = entry.t;
        if( ton > entry.t )
        {
            result << entry.t;
            result += " ";
            result << entry.V.typ;
            result += " ";
        }
        else
        {
            std::cout << "WARNING: t_on is smaller than rising waveform. " << std::endl;
            break;
        }
    }
    for( auto entry : m_fallingWaveforms.at( 1 )->m_table.m_entries )
    {
        if( toff > entry.t )
        {
            result << entry.t + ton;
            result += " ";
            result << entry.V.typ;
            result += " ";
        }
        else
        {
            std::cout << "WARNING: t_off is smaller than falling waveform. " << std::endl;
            break;
        }
    }
    result += ") \n";

    *aDest = result;

    return status;
}

wxString KIBIS_PIN::getKuKd( KIBIS_MODEL* aModel )
{
    double ton = 20e-9;
    double toff = 60e-9;

    wxString simul = "";
    simul += "*THIS IS NOT A VALID SPICE MODEL.\n";
    simul += "*This part is intended to be executed by Kibis internally.\n";
    simul += "*You should not be able to read this.\n\n";
    simul += ".SUBCKT DRIVER POWER GND OUT \n"; // 1: POWER, 2:GND, 3:OUT

    simul += "RPIN 1 OUT ";
    simul << R_pin.typ;
    simul += "\n";
    simul += "LPIN 2 1 ";
    simul << L_pin.typ;
    simul += "\n";
    simul += "CPIN OUT GND ";
    simul << C_pin.typ;
    simul += "\n";

    simul += "\n";
    simul += "CCPOMP 2 GND ";
    simul << aModel->m_C_comp.typ;
    simul += "\n";
    simul += aModel->m_GNDClamp.Spice( 1, "DIE", "GND1", "GNDClampDiode" );
    simul += aModel->m_POWERClamp.Spice( 2, "DIE", "POWER1", "POWERClampDiode" );
    simul += aModel->m_pulldown.Spice( 3, "DIE", "GND2", "Pulldown" );
    simul += aModel->m_pullup.Spice( 4, "DIE", "POWER2", "Pullup" );


    simul += "vin DIE GND pwl ( ";

    std::vector<std::pair<IbisWaveform*, IbisWaveform*>> wfPairs = aModel->waveformPairs();

    if( wfPairs.size() < 1 )
    {
        std::cout << "Model has no waveform pair, using [Ramp] instead, poor accuracy" << std::endl;
    }
    else if( wfPairs.size() == 1 )
    {
    }
    else
    {
        if( wfPairs.size() > 2 )
        {
            std::cout << "Model has more than 2 waveform pairs, using the first two." << std::endl;
        }
    }

    double lastT;
    for( auto entry : aModel->m_risingWaveforms.at( 0 )->m_table.m_entries )
    {
        lastT = entry.t;
        if( ton > entry.t )
        {
            simul << entry.t;
            simul += " ";
            simul << entry.V.typ;
            simul += " ";
        }
        else
        {
            std::cout << "WARNING: t_on is smaller than rising waveform. " << std::endl;
            break;
        }
    }
    for( auto entry : aModel->m_fallingWaveforms.at( 1 )->m_table.m_entries )
    {
        if( toff > entry.t )
        {
            simul << entry.t + ton;
            simul += " ";
            simul << entry.V.typ;
            simul += " ";
        }
        else
        {
            std::cout << "WARNING: t_off is smaller than falling waveform. " << std::endl;
            break;
        }
    }
    simul += ") \n";
    simul += "\n.ENDS DRIVER\n\n";
    simul += "\n x1 3 0 1 DRIVER \n";
    // In IBIS Currents are positive when entering the component
    // In SPICE, currents are positive when flowing from the first terminal to the second terminal
    // => The first terminal is outside, the second inside
    simul += "VCC 3 0 1.8\n";
    //simul += "Vpin x1.DIE 0 1 \n";
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
    simul += "run \n";
    simul += "plot v(x1.DIE) i(VmeasIout) i(VmeasPD) i(VmeasPU) i(VmeasPC) i(VmeasGC) "
             "v(x1.POWER2)\n";
    simul += "plot v(KU) v(KD)\n";
    simul += ".endc \n";
    simul += ".end \n";


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

    result += tmp;
    result += "\n.ENDS DRIVER\n\n";

    *aDest = result;

    return true;
}