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
    if( m_GNDClamp.m_entries.size() > 0 )
    {
        result += "a1 %vd(GND DIE) %id(GND DIE) GNDClampDiode\n";
        result += "\n";
        result += ".model GNDClampDiode pwl(\n+ x_array=[";
        for( auto entry : m_GNDClamp.m_entries )
        {
            result << entry.V;
            result += " ";
        }
        result += "]\n+ y_array=[";
        for( auto entry : m_GNDClamp.m_entries )
        {
            result << entry.I.typ;
            result += " ";
        }
        result += "]\n+ input_domain=0.05 fraction=TRUE)\n";
    }

    result += "\n";
    if( m_POWERClamp.m_entries.size() > 0 )
    {
        result += "a2 %vd(POWER DIE) %id(POWER DIE) POWERClampDiode\n";
        result += "\n";
        result += ".model POWERClampDiode pwl(\n+ x_array=[";
        for( auto entry : m_POWERClamp.m_entries )
        {
            result << entry.V;
            result += " ";
        }
        result += "]\n+ y_array=[";
        for( auto entry : m_POWERClamp.m_entries )
        {
            result << entry.I.typ;
            result += " ";
        }
        result += "]\n+ input_domain=0.05 fraction=TRUE)\n";
    }

    result += "\n";
    if( m_pulldown.m_entries.size() > 0 )
    {
        result += "a3 %vd(GND DIE) %id(GND DIE) pulldown\n";
        result += "\n";
        result += ".model pulldown pwl(\n+ x_array=[";
        for( auto entry : m_pulldown.m_entries )
        {
            result << entry.V;
            result += " ";
        }
        result += "]\n+ y_array=[";
        for( auto entry : m_pulldown.m_entries )
        {
            result << entry.I.typ;
            result += " ";
        }
        result += "]\n+ input_domain=0.05 fraction=TRUE)\n";
    }

    result += "\n";
    if( m_pulldown.m_entries.size() > 0 )
    {
        result += "a3 %vd(POWER DIE) %id(POWER DIE) pullup\n";
        result += "\n";
        result += ".model pullup pwl(\n+ x_array=[";
        for( auto entry : m_pulldown.m_entries )
        {
            result << entry.V;
            result += " ";
        }
        result += "]\n+ y_array=[";
        for( auto entry : m_pulldown.m_entries )
        {
            result << entry.I.typ;
            result += " ";
        }
        result += "]\n+ input_domain=0.05 fraction=TRUE)\n";
    }


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