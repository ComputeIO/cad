#include "../../pcbnew/signalIntegrity/kibis.h"
#include <wx/textfile.h>

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
    wxString*  tmp = new wxString();
    pin->writeSpiceDriver( tmp, pin->m_models.at( 0 ) );

    wxTextFile file( "output.sp" );
    if( file.Exists() )
    {
        file.Clear();
    }
    else
    {
        file.Create();
    }
    file.AddLine( *tmp );

    wxString simul = "";
    simul += "\n x1 3 0 1 DRIVER \n";
    //simul += "Cload 1 0 100p\n";
    simul += "VPOWER 3 0 1.8\n";
    simul += ".tran 0.1n 7n \n";
    simul += ".option xmu=0.49  \n";
    simul += ".control run \n";
    simul += "run \n";
    simul += "plot v(1) \n";
    //simul += "plot v(x1.KU) v(x1.KD) v(1) v(x1.DIEBUFF) \n";
    simul += ".endc \n";
    simul += ".end \n";


    file.AddLine( simul );

    file.Write();


    std::cout << "Done" << std::endl;

    return 1;
}