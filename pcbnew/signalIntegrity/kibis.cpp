#include "kibis.h"
#include "ibis2kibis.h"

bool KibisFromFile( wxString aFileName, std::vector<KIBIS_COMPONENT*>* aDest )
{
    IbisParser* parser = new IbisParser();
    IbisFile*   file = new IbisFile();
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
