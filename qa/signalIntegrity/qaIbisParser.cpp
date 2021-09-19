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
    wxString*  tmp1 = new wxString();
    wxString*  tmp2 = new wxString();
    wxString*  tmp3 = new wxString();
    pin->writeSpiceDriver( tmp1, "driver_typ", pin->m_models.at( 0 ), IBIS_CORNER::MAX,
                           IBIS_CORNER::TYP, KIBIS_ACCURACY::LEVEL_0 );
    pin->writeSpiceDriver( tmp2, "driver_min", pin->m_models.at( 0 ), IBIS_CORNER::MAX,
                           IBIS_CORNER::TYP, KIBIS_ACCURACY::LEVEL_1 );
    pin->writeSpiceDriver( tmp3, "driver_max", pin->m_models.at( 0 ), IBIS_CORNER::MAX,
                           IBIS_CORNER::TYP, KIBIS_ACCURACY::LEVEL_2 );

    wxTextFile file( "output.sp" );
    if( file.Exists() )
    {
        file.Clear();
    }
    else
    {
        file.Create();
    }
    file.AddLine( *tmp1 );
    file.AddLine( *tmp2 );
    file.AddLine( *tmp3 );

    wxString simul = "";
    simul += "\n x1 3 0 1 driver_typ \n";
    simul += "\n x2 3 0 4 driver_min \n";
    simul += "\n x3 3 0 5 driver_max \n";
    //simul += "Cload 1 0 100p\n";
    simul += "VPOWER 3 0 1.8\n";
    simul += "R0 1 0 1000\n";
    simul += "R1 4 0 1000\n";
    simul += "R2 5 0 1000\n";
    simul += ".tran 0.1n 40n \n";
    simul += ".option xmu=0.49  \n";
    simul += ".control run \n";
    simul += "run \n";
    simul += "plot v(1) v(4) v(5) \n";
    //simul += "plot v(x1.KU) v(x1.KD) v(1) v(x1.DIEBUFF) \n";
    simul += ".endc \n";
    simul += ".end \n";


    file.AddLine( simul );

    file.Write();


    std::cout << "Done" << std::endl;

    return 1;
}