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

bool KIBIS_PIN::writeSpiceDriver( wxString* aDest )
{
    double   ton = 20e-9;
    double   toff = 60e-9;
    wxString result = "\n.SUBCKT DRIVER 1 2 3 \n"; // 1: POWER, 2:GND, 3:OUT
    result += "vin 3 0 pwl ( ";                    // 1: POWER, 2:GND, 3:OUT
    double lastT;
    std::cout << "Adding rising waveform " << std::endl;
    for( auto entry : m_models.at( 0 )->m_risingWaveforms.at( 0 )->m_table.m_entries )
    {
        std::cout << ".";
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
    std::cout << "Adding falling waveform " << std::endl;
    for( auto entry : m_models.at( 0 )->m_fallingWaveforms.at( 0 )->m_table.m_entries )
    {
        std::cout << ".";

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
    result += ".ENDS DRIVER\n\n";
    std::cout << result << std::endl;

    return true;
}