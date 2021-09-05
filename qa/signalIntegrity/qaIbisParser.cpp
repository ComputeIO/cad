#include "../../pcbnew/signalIntegrity/kibis.h"

int main( void )
{
    std::vector<KIBIS_COMPONENT*> components1;
    KibisFromFile( "ibis_v1_1.ibs", &components1 );
    std::vector<KIBIS_COMPONENT*> components2;
    KibisFromFile( "ibis_v2_1.ibs", &components2 );
    std::vector<KIBIS_COMPONENT*> components3;
    KibisFromFile( "ibis_v2_1.pkg", &components3 );
    std::vector<KIBIS_COMPONENT*> components4;
    KibisFromFile( "sn74lvc541a.ibs", &components4 );

    KIBIS_COMPONENT* comp = components4.at( 0 );

    std::cout << "Component: " << comp->m_name << std::endl;

    for( KIBIS_PIN* pin : comp->m_pins )
    {
        std::cout << "--Pin: " << pin->m_pinNumber << " " << pin->m_signalName << std::endl;
    }


    KIBIS_PIN* pin = comp->getPin( "11" );
    wxString*  tmp;
    pin->writeSpiceDriver( tmp );

    std::cout << "Done" << std::endl;
    return 1;
}