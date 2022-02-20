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


    KIBIS_PIN* pin1 = comp->getPin( "11" );
    KIBIS_PIN* pin2 = comp->getPin( "2" );

    std::cout << "pin1 name: " << pin1->m_signalName << std::endl;
    std::cout << "pin2 name: " << pin2->m_signalName << std::endl;
    std::cout << "pin1 model length: " << pin1->m_models.size()<< std::endl;
    std::cout << "pin2 model length: " << pin2->m_models.size()<< std::endl;
    std::string*  tmp1 = new std::string();
    std::string*  tmp2 = new std::string();
    std::string*  tmp3 = new std::string();
    std::string*  tmp4 = new std::string();

    KIBIS_WAVEFORM_RECTANGULAR* wave = new KIBIS_WAVEFORM_RECTANGULAR();
    wave->m_ton = 12e-9;
    wave->m_toff = 12e-9;
    wave->m_cycles = 5;

    std::cout << "WAVEFORM TYPE IN QA: " << wave->GetType() << std::endl;
    std::cout << pin2->m_models.at(0)->m_name << std::endl;
    pin2->writeSpiceDevice( tmp4, "device_typ", pin2->m_models.at( 0 ), IBIS_CORNER::TYP,
                            IBIS_CORNER::TYP );
    pin1->writeSpiceDriver( tmp1, "driver_typ", pin1->m_models.at( 0 ), IBIS_CORNER::TYP,
                            IBIS_CORNER::TYP, KIBIS_ACCURACY::LEVEL_1, wave, wave->GetType() );
    pin1->writeSpiceDriver( tmp2, "driver_min", pin1->m_models.at( 0 ), IBIS_CORNER::MIN,
                            IBIS_CORNER::MIN, KIBIS_ACCURACY::LEVEL_1, wave,
                            KIBIS_WAVEFORM_TYPE::NONE );
    pin1->writeSpiceDriver( tmp3, "driver_max", pin1->m_models.at( 0 ), IBIS_CORNER::MAX,
                            IBIS_CORNER::MAX, KIBIS_ACCURACY::LEVEL_2, wave,
                            KIBIS_WAVEFORM_TYPE::NONE );

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
    file.AddLine( *tmp4 );

    wxString simul = "";
    simul += "\n x1 OUT_1 0 DIE_1 driver_typ \n";
    simul += "R1 OUT_1 GND 1 00\n";
    simul += ".tran 0.1n 40n \n";
    simul += ".option xmu=0.49  \n";
    simul += ".control run \n";
    simul += "run \n";
    simul += "plot v(OUT_1) v(DIE_1) \n";
    //simul += "plot v(x1.KU) v(x1.KD) v(1) v(x1.DIEBUFF) \n";
    simul += ".endc \n";
    simul += ".end \n";


    file.AddLine( simul );

    file.Write();


    std::cout << "Done" << std::endl;

    return 1;
}