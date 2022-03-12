/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Fabien Corona f.corona<at>laposte.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include "kibis.h"
#include "ibisParser.h"
#include <sstream>

std::string doubleToString( double aNumber )
{
    std::ostringstream ss;
    ss.setf( std::ios_base::scientific, std::ios_base::floatfield );
    ss << aNumber;
    return ss.str();
}

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

KIBIS::KIBIS( std::string aFileName ) : KIBIS_ANY( this )
{
    IbisParser* parser = new IbisParser();
    IbisFile*   file = new IbisFile();
    parser->m_parrot = false;
    parser->parseFile( aFileName, file );

    bool status = true;

    m_file = new KIBIS_FILE( this, parser );

    for( IbisModel iModel : parser->m_ibisFile->m_models )
    {
        KIBIS_MODEL* kModel = new KIBIS_MODEL( this, &iModel, parser );
        status &= kModel->m_valid;
        m_models.push_back( kModel );
    }

    for( IbisComponent iComponent : parser->m_ibisFile->m_components )
    {
        KIBIS_COMPONENT* kComponent = new KIBIS_COMPONENT( this, &iComponent, parser );
        status &= kComponent->m_valid;
        m_components.push_back( kComponent );
    }

    m_valid = status;
}


KIBIS_FILE::KIBIS_FILE( KIBIS* aTopLevel, IbisParser* aParser ) : KIBIS_ANY( aTopLevel )
{
    bool status = true;

    if( aParser == nullptr )
    {
        std::cerr << "Internal Error: Ibis -> Kibis, a pointer is nullptr" << std::endl;
        status = false;
    }
    else
    {
        m_fileName = aParser->m_ibisFile->m_header.m_fileName;
        m_fileRev = aParser->m_ibisFile->m_header.m_fileRevision;
        m_ibisVersion = aParser->m_ibisFile->m_header.m_ibisVersion;
        m_date = aParser->m_ibisFile->m_header.m_date;
        m_notes = aParser->m_ibisFile->m_header.m_notes;
        m_disclaimer = aParser->m_ibisFile->m_header.m_disclaimer;
        m_copyright = aParser->m_ibisFile->m_header.m_copyright;
    }

    m_valid = status;
}

KIBIS_PIN::KIBIS_PIN( KIBIS* aTopLevel, IbisComponentPin* aPin, IbisComponentPackage* aPackage,
                      IbisParser* aParser, std::vector<KIBIS_MODEL*> aModels ) :
        KIBIS_ANY( aTopLevel )
{
    m_signalName = aPin->m_signalName;
    m_pinNumber = aPin->m_pinName;

    R_pin = aPackage->m_Rpkg;
    L_pin = aPackage->m_Lpkg;
    C_pin = aPackage->m_Cpkg;

    // The values listed in the [Pin] description section override the default
    // values defined in [Package]

    // @TODO : Reading the IBIS standard, I can't figure out if we are supposed
    // to replace typ, min, and max, or just the typ ?

    if( !std::isnan( aPin->m_Lpin ) )
    {
        R_pin.value[IBIS_CORNER::TYP] = aPin->m_Rpin;
        R_pin.value[IBIS_CORNER::MIN] = aPin->m_Rpin;
        R_pin.value[IBIS_CORNER::MAX] = aPin->m_Rpin;
    }
    if( !std::isnan( aPin->m_Lpin ) )
    {
        L_pin.value[IBIS_CORNER::TYP] = aPin->m_Lpin;
        L_pin.value[IBIS_CORNER::MIN] = aPin->m_Lpin;
        L_pin.value[IBIS_CORNER::MAX] = aPin->m_Lpin;
    }
    if( !std::isnan( aPin->m_Cpin ) )
    {
        C_pin.value[IBIS_CORNER::TYP] = aPin->m_Cpin;
        C_pin.value[IBIS_CORNER::MIN] = aPin->m_Cpin;
        C_pin.value[IBIS_CORNER::MAX] = aPin->m_Cpin;
    }

    bool                     modelSelected = false;
    std::vector<std::string> listOfModels;

    for( IbisModelSelector modelSelector : aParser->m_ibisFile->m_modelSelectors )
    {
        if( !strcmp( modelSelector.m_name.c_str(), aPin->m_modelName.c_str() ) )
        {
            for( IbisModelSelectorEntry model : modelSelector.m_models )
            {
                listOfModels.push_back( model.m_modelName );
            }
            modelSelected = true;
            break;
        }
    }
    if( !modelSelected )
    {
        listOfModels.push_back( aPin->m_modelName );
    }

    for( std::string modelName : listOfModels )
    {
        for( KIBIS_MODEL* model : aModels )
        {
            if( !strcmp( model->m_name.c_str(), modelName.c_str() ) )
            {
                m_models.push_back( model );
            }
        }
    }
}

KIBIS_MODEL::KIBIS_MODEL( KIBIS* aTopLevel, IbisModel* aSource, IbisParser* aParser ) :
        KIBIS_ANY( aTopLevel )
{
    bool status = true;

    if( aSource == nullptr || aParser == nullptr )
    {
        std::cerr << "Internal Error: Ibis -> Kibis, a pointer is nullptr" << std::endl;
        status = false;
    }
    else
    {
        m_name = aSource->m_name;
        m_type = aSource->m_type;

        m_description = std::string( "No description available." );

        for( IbisModelSelector modelSelector : aParser->m_ibisFile->m_modelSelectors )
        {
            for( IbisModelSelectorEntry entry : modelSelector.m_models )
            {
                if( !strcmp( entry.m_modelName.c_str(), m_name.c_str() ) )
                {
                    m_description = entry.m_modelDescription;
                }
            }
        }

        m_vinh = aSource->m_vinh;
        m_vinl = aSource->m_vinl;
        m_vref = aSource->m_vref;
        m_rref = aSource->m_rref;
        m_cref = aSource->m_cref;
        m_vmeas = aSource->m_vmeas;

        m_enable = aSource->m_enable;
        m_polarity = aSource->m_polarity;

        m_ramp = aSource->m_ramp;
        m_risingWaveforms = aSource->m_risingWaveforms;
        m_fallingWaveforms = aSource->m_fallingWaveforms;
        m_GNDClamp = aSource->m_GNDClamp;
        m_GNDClampReference = aSource->m_GNDClampReference;
        m_POWERClamp = aSource->m_POWERClamp;
        m_POWERClampReference = aSource->m_POWERClampReference;

        m_C_comp = aSource->m_C_comp;
        m_voltageRange = aSource->m_voltageRange;
        m_temperatureRange = aSource->m_temperatureRange;
        m_pullupReference = aSource->m_pullupReference;
        m_pulldownReference = aSource->m_pulldownReference;

        m_Rgnd = aSource->m_Rgnd;
        m_Rpower = aSource->m_Rpower;
        m_Rac = aSource->m_Rac;
        m_Cac = aSource->m_Cac;
        m_pullup = aSource->m_pullup;
        m_pulldown = aSource->m_pulldown;
    }
    m_valid = status;
}

KIBIS_COMPONENT::KIBIS_COMPONENT( KIBIS* aTopLevel, IbisComponent* aSource, IbisParser* aParser ) :
        KIBIS_ANY( aTopLevel )
{
    bool status = true;

    m_name = aSource->m_name;
    m_manufacturer = aSource->m_manufacturer;
    m_topLevel = aTopLevel;

    for( IbisComponentPin iPin : aSource->m_pins )
    {
        if( iPin.m_dummy )
        {
            continue;
        }

        KIBIS_PIN* kPin = new KIBIS_PIN( aTopLevel, &iPin, &( aSource->m_package ), aParser,
                                         m_topLevel->m_models );
        status &= kPin->m_valid;
        m_pins.push_back( kPin );
    }

    m_valid = status;
}

KIBIS_PIN* KIBIS_COMPONENT::getPin( std::string aPinNumber )
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

                std::cout << "WAVEFORM PAIR :" << std::endl;
                std::cout << wf1->m_R_fixture << std::endl;
                std::cout << wf1->m_L_fixture << std::endl;
                std::cout << wf1->m_C_fixture << std::endl;
                std::cout << wf1->m_V_fixture << std::endl;
            }
        }
    }

    return pairs;
}

std::string KIBIS_MODEL::SpiceDie( IBIS_CORNER aSupply, IBIS_CORNER aSpeed )
{
    std::string result;

    result = "\n";
    result += "VPWR POWER GND ";
    result += doubleToString( m_voltageRange.value[ aSupply ] );
    result += "\n";
    result += "CCPOMP DIE GND ";
    result += doubleToString( m_C_comp.value[ReverseLogic( aSpeed )] );
    result += "\n";
    result += m_GNDClamp.Spice( 1, "DIE", "GND", "GNDClampDiode", aSupply );
    result += m_POWERClamp.Spice( 2, "DIE", "POWER", "POWERClampDiode", aSupply );
    result += m_pulldown.Spice( 3, "DIEBUFF", "GND2", "Pulldown", aSupply );
    result += m_pullup.Spice( 4, "POWER2", "DIEBUFF", "Pullup", aSupply );
    result += "BDIEBUFF DIEBUFF GND v=v(DIE)\n";

    result += "VmeasPD GND GND2 0\n";
    result += "VmeasPU POWER POWER2 0\n";
    result += "BKU POWER DIE i=( i(VmeasPD) * v(KD) )\n";
    result += "BKD GND DIE i=( -i(VmeasPU) * v(KU) )\n";
    result += "BDIE_M DIE_M GND v=v(DIE)\n";


    return result;
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
    return m_pulldown.m_entries.size() > 0;
}
bool KIBIS_MODEL::HasPullup()
{
    return m_pullup.m_entries.size() > 0;
}
bool KIBIS_MODEL::HasGNDClamp()
{
    return m_GNDClamp.m_entries.size() > 0;
}
bool KIBIS_MODEL::HasPOWERClamp()
{
    return m_POWERClamp.m_entries.size() > 0;
}

std::string KIBIS_MODEL::generateSquareWave( std::string aNode1, std::string aNode2,
                                          KIBIS_WAVEFORM_RECTANGULAR*             aWave,
                                          std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                          IBIS_CORNER                             aSupply )
{
    std::string simul;

    IbisWaveform risingWF = TrimWaveform( aPair.first );
    IbisWaveform fallingWF = TrimWaveform( aPair.second );
    std::cout << "Times : " << aWave->m_ton << "  " << aWave->m_toff << std::endl;

    if( aWave->m_ton < risingWF.m_table.m_entries.at( risingWF.m_table.m_entries.size() - 1 ).t )
    {
        std::cerr << "WARNING: rising edge is longer than on time." << std::endl;
    }

    if( aWave->m_toff < fallingWF.m_table.m_entries.at( fallingWF.m_table.m_entries.size() - 1 ).t )
    {
        std::cerr << "WARNING: falling edge is longer than on time." << std::endl;
    }

    for( int i = 0; i < aWave->m_cycles; i++ )
    {
        simul += "Vrise";
        simul += std::to_string( i );
        simul += " rise";
        simul += std::to_string( i );
        simul += " ";
        simul += aNode2;
        simul += " pwl ( ";

        for( auto entry : risingWF.m_table.m_entries )
        {
            simul += doubleToString( entry.t + i * ( aWave->m_ton + aWave->m_toff ) );
            simul += " ";
            simul += doubleToString( entry.V.value[aSupply] );
            simul += " ";
        }
        simul += ")\n";
    }

    for( int i = 0; i < aWave->m_cycles; i++ )
    {
        simul += "Vfall";
        simul += std::to_string( i );
        simul += " fall";
        simul += std::to_string( i );
        simul += " ";
        simul += aNode2;
        simul += " pwl ( ";

        for( auto entry : fallingWF.m_table.m_entries )
        {
            simul += doubleToString( entry.t + i * ( aWave->m_ton + aWave->m_toff ) + aWave->m_ton );
            simul += " ";
            simul += doubleToString( entry.V.value[aSupply] );
            simul += " ";
        }
        simul += ")\n";
    }

    simul += "bin ";
    simul += aNode1;
    simul += " ";
    simul += aNode2;
    simul += " v=(";

    for( int i = 0; i < aWave->m_cycles; i++ )
    {
        simul += " v( rise";
        simul += std::to_string( i );
        simul += " ) + v( fall";
        simul += std::to_string( i );
        simul += ") ";
        simul += "+";
    }
    simul += doubleToString( aPair.first->m_table.m_entries.at( 0 ).V.value[aSupply] ); // Add DC bias

    simul += ")\n";
    return simul;
}

std::string KIBIS_PIN::addDie( KIBIS_MODEL* aModel, IBIS_CORNER aSupply, int aIndex )
{
    std::string simul;

    std::string GC_GND = "GC_GND";
    std::string PC_PWR = "PC_PWR";
    std::string PU_PWR = "PU_PWR";
    std::string PD_GND = "PD_GND";
    std::string DIE = "DIE";

    GC_GND += std::to_string( aIndex );
    PC_PWR += std::to_string( aIndex );
    PU_PWR += std::to_string( aIndex );
    PD_GND += std::to_string( aIndex );
    DIE += std::to_string( aIndex );


    std::string GC = "GC";
    std::string PC = "PC";
    std::string PU = "PU";
    std::string PD = "PD";

    GC += std::to_string( aIndex );
    PC += std::to_string( aIndex );
    PU += std::to_string( aIndex );
    PD += std::to_string( aIndex );

    if( aModel->HasGNDClamp() )
    {
        simul += aModel->m_GNDClamp.Spice( aIndex * 4 + 1, DIE, GC_GND, GC, aSupply );
    }
    if( aModel->HasPOWERClamp() )
    {
        simul += aModel->m_POWERClamp.Spice( aIndex * 4 + 2, DIE, PC_PWR, PC, aSupply );
    }
    if( aModel->HasPulldown() )
    {
        simul += aModel->m_pulldown.Spice( aIndex * 4 + 3, DIE, PD_GND, PD, aSupply );
    }
    if( aModel->HasPullup() )
    {
        simul += aModel->m_pullup.Spice( aIndex * 4 + 4, PU_PWR, DIE, PU, aSupply );
    }

    return simul;
}

void KIBIS_PIN::getKuKdFromFile( std::string* aSimul )
{
    // @TODO
    // that's not the best way to do, but ¯\_(ツ)_/¯
    std::ifstream in( "temp_input.spice" );

    std::remove( "temp_input.spice" );
    std::remove( "temp_output.spice" );

    std::ofstream file( "temp_input.spice" );

    file << *aSimul;

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
    std::remove( "temp_input.spice" );
    std::remove( "temp_output.spice" );

    // @TODO : this is the end of the dirty code

    m_Ku = ku;
    m_Kd = kd;
    m_t = t;
}

std::string KIBIS_PIN::KuKdDriver( KIBIS_MODEL* aModel, std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType,
                                IBIS_CORNER aSupply, IBIS_CORNER aSpeed, int index )
{
    std::string simul = "";

    simul += "*THIS IS NOT A VALID SPICE MODEL.\n";
    simul += "*This part is intended to be executed by Kibis internally.\n";
    simul += "*You should not be able to read this.\n\n";

    simul += ".SUBCKT DRIVER";
    simul += std::to_string( index );
    simul += " POWER GND PIN \n"; // 1: POWER, 2:GND, 3:PIN

    if( ( aPair.first->m_R_dut == 0 ) && ( aPair.first->m_L_dut == 0 )
                && ( aPair.first->m_C_dut == 0 )
        || true )
    {
        simul += "Vdummy 2 PIN 0\n";
    }
    {
        std::cout << "R:" << aPair.first->m_R_dut << std::endl;
        std::cout << "L:" << aPair.first->m_L_dut << std::endl;
        std::cout << "C:" << aPair.first->m_C_dut << std::endl;
        /*
        simul += "RPIN 1 PIN ";
        simul << aPair.first->m_R_dut;
        simul += "\n";
        simul += "LPIN 2 1 ";
        simul << aPair.first->m_L_dut;
        simul += "\n";
        simul += "CPIN PIN GND ";
        simul << aPair.first->m_C_dut;
        simul += "\n";
        */
        std::cerr
                << "WARNING : I can't yet use DUT values. https://ibis.org/summits/nov16a/chen.pdf"
                << std::endl;
    }

    simul += "\n";
    simul += "CCPOMP 2 GND ";
    simul += doubleToString( aModel->m_C_comp.value[aSpeed] ); //@TODO: Check the corner ?
    simul += "\n";
    switch( aWaveType )
    {
    case KIBIS_WAVEFORM_TYPE::RECTANGULAR:
        simul += aModel->generateSquareWave(
                "DIE0", "GND", static_cast<KIBIS_WAVEFORM_RECTANGULAR*>( aWave ), aPair, aSupply );
        break;
    case KIBIS_WAVEFORM_TYPE::NONE:
    default: std::cout << "NO WAVEFORM" << std::endl; break;
    }
    simul += addDie( aModel, aSupply, 0 );

    simul += "\n.ENDS DRIVER\n\n";
    return simul;
}

std::string KIBIS_PIN::getKuKdOneWaveform( KIBIS_MODEL*                            aModel,
                                        std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                        KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType,
                                        IBIS_CORNER aSupply, IBIS_CORNER aSpeed )
{
    std::string simul = "";

    if( aWaveType == KIBIS_WAVEFORM_TYPE::NONE )
    {
        //@TODO , there could be some current flowing through pullup / pulldown transistors, even when off
        std::vector<double> ku, kd, t;
        ku.push_back( 0 );
        kd.push_back( 0 );
        t.push_back( 0 );
        m_Ku = ku;
        m_Kd = kd;
        m_t = t;
    }
    else
    {
        simul += KuKdDriver( aModel, aPair, aWave, aWaveType, aSupply, aSpeed, 0 );
        simul += "\n x1 3 0 1 DRIVER0 \n";

        simul += "VCC 3 0 ";
        simul += doubleToString( aModel->m_voltageRange.value[aSupply] );
        simul += "\n";
        //simul += "Vpin x1.DIE 0 1 \n"
        simul += "Lfixture 1 4 ";
        simul += doubleToString( aPair.first->m_L_fixture );
        simul += "\n";
        simul += "Rfixture 4 5 ";
        simul += doubleToString( aPair.first->m_R_fixture );
        simul += "\n";
        simul += "Cfixture 4 0 ";
        simul += doubleToString( aPair.first->m_C_fixture );
        simul += "\n";
        simul += "Vfixture 5 0 ";
        simul += doubleToString( aPair.first->m_V_fixture );
        simul += "\n";
        simul += "VmeasIout x1.2 x1.DIE0 0\n";
        simul += "VmeasPD 0 x1.PD_GND0 0\n";
        simul += "VmeasPU x1.PU_PWR0 3 0\n";
        simul += "VmeasPC x1.PC_PWR0 3 0\n";
        simul += "VmeasGC 0 x1.GC_PWR0 0\n";

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
            std::cout << "ERROR: Driver needs at least a pullup or a pulldown" << std::endl;
        }


        simul += ".tran 0.1n 100n \n";
        simul += ".option xmu=0.49  \n";
        //simul += ".dc Vpin -5 5 0.1\n";
        simul += ".control run \n";
        simul += "set filetype=ascii\n";
        simul += "run \n";
        //simul += "plot v(x1.DIE) i(VmeasIout) i(VmeasPD) i(VmeasPU) i(VmeasPC) i(VmeasGC) "
        //         "v(x1.POWER2)\n";
        //simul += "plot v(KU) v(KD)\n";
        simul += "write temp_output.spice v(KU) v(KD)\n"; // @TODO we might want to remove this...
        simul += "quit\n";
        simul += ".endc \n";
        simul += ".end \n";

        getKuKdFromFile( &simul );
    }
    return simul;
}

void KIBIS_PIN::getKuKdNoWaveform( KIBIS_MODEL* aModel, KIBIS_WAVEFORM* aWave,
                                   KIBIS_WAVEFORM_TYPE aWaveType, IBIS_CORNER aSupply )
{
    std::vector<double> ku, kd, t;

    switch( aWaveType )
    {
    case KIBIS_WAVEFORM_TYPE::RECTANGULAR:
    {
        KIBIS_WAVEFORM_RECTANGULAR* rectWave = static_cast<KIBIS_WAVEFORM_RECTANGULAR*>( aWave );
        for( int i = 0; i < 10; i++ )
        {
            ku.push_back( 0 );
            kd.push_back( 1 );
            t.push_back( ( rectWave->m_ton + rectWave->m_toff ) * i );
            ku.push_back( 1 );
            kd.push_back( 0 );
            t.push_back( ( rectWave->m_ton + rectWave->m_toff ) * i
                         + aModel->m_ramp.m_rising.value[aSupply].m_dt
                                   / 0.6 ); // 0.6 because ibis only gives 20%-80% time
            ku.push_back( 1 );
            kd.push_back( 0 );
            t.push_back( ( rectWave->m_ton + rectWave->m_toff ) * i + rectWave->m_toff );
            ku.push_back( 0 );
            kd.push_back( 1 );
            t.push_back( ( rectWave->m_ton + rectWave->m_toff ) * i + rectWave->m_toff
                         + aModel->m_ramp.m_falling.value[aSupply].m_dt / 0.6 );
        }
        break;
    }
    case KIBIS_WAVEFORM_TYPE::NONE:
    default:
        ku.push_back( 0 );
        kd.push_back( 0 );
        t.push_back( 0 );
    }
    m_Ku = ku;
    m_Kd = kd;
    m_t = t;
}


std::string KIBIS_PIN::getKuKdTwoWaveforms( KIBIS_MODEL*                            aModel,
                                         std::pair<IbisWaveform*, IbisWaveform*> aPair1,
                                         std::pair<IbisWaveform*, IbisWaveform*> aPair2,
                                         KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType,
                                         IBIS_CORNER aSupply, IBIS_CORNER aSpeed )
{
    std::string simul = "";

    if( aWaveType == KIBIS_WAVEFORM_TYPE::NONE )
    {
        //@TODO , there could be some current flowing through pullup / pulldown transistors, even when off
        std::vector<double> ku, kd, t;
        ku.push_back( 0 );
        kd.push_back( 0 );
        t.push_back( 0 );
        m_Ku = ku;
        m_Kd = kd;
        m_t = t;
    }
    else
    {
        simul += KuKdDriver( aModel, aPair1, aWave, aWaveType, aSupply, aSpeed, 0 );
        simul += KuKdDriver( aModel, aPair2, aWave, aWaveType, aSupply, aSpeed, 1 );
        simul += "\n x1 3 0 1 DRIVER0 \n";

        simul += "VCC 3 0 ";
        simul += doubleToString( aModel->m_voltageRange.value[aSupply] );
        simul += "\n";
        //simul += "Vpin x1.DIE 0 1 \n"
        simul += "Lfixture0 1 4 ";
        simul += doubleToString( aPair1.first->m_L_fixture );
        simul += "\n";
        simul += "Rfixture0 4 5 ";
        simul += doubleToString( aPair1.first->m_R_fixture );
        simul += "\n";
        simul += "Cfixture0 4 0 ";
        simul += doubleToString( aPair1.first->m_C_fixture );
        simul += "\n";
        simul += "Vfixture0 5 0 ";
        simul += doubleToString( aPair1.first->m_V_fixture );
        simul += "\n";
        simul += "VmeasIout0 x1.2 x1.DIE0 0\n";
        simul += "VmeasPD0 0 x1.PD_GND0 0\n";
        simul += "VmeasPU0 x1.PU_PWR0 3 0\n";
        simul += "VmeasPC0 x1.PC_PWR0 3 0\n";
        simul += "VmeasGC0 0 x1.GC_PWR0 0\n";


        simul += "\n x2 3 0 7 DRIVER1 \n";
        //simul += "Vpin x1.DIE 0 1 \n"
        simul += "Lfixture1 7 8 ";
        simul += doubleToString( aPair2.first->m_L_fixture );
        simul += "\n";
        simul += "Rfixture1 8 9 ";
        simul += doubleToString( aPair2.first->m_R_fixture );
        simul += "\n";
        simul += "Cfixture1 8 0 ";
        simul += doubleToString( aPair2.first->m_C_fixture );
        simul += "\n";
        simul += "Vfixture1 9 0 ";
        simul += doubleToString( aPair2.first->m_V_fixture );
        simul += "\n";
        simul += "VmeasIout1 x2.2 x2.DIE0 0\n";
        simul += "VmeasPD1 0 x2.PD_GND0 0\n";
        simul += "VmeasPU1 x2.PU_PWR0 3 0\n";
        simul += "VmeasPC1 x2.PC_PWR0 3 0\n";
        simul += "VmeasGC1 0 x2.GC_PWR0 0\n";

        if( aModel->HasPullup() && aModel->HasPulldown() )
        {
            simul +=
                    "Bku KU 0 v=(  ( i(VmeasPD1) * ( i(VmeasIout0) + i(VmeasPC0) + i(VmeasGC0) ) - "
                    "i(VmeasPD0) * ( i(VmeasIout1) + i(VmeasPC1) + i(VmeasGC1) )  )/ ( i(VmeasPU1) "
                    "* "
                    "i(VmeasPD0) - i(VmeasPU0) * i(VmeasPD1)  ) )\n";
            simul +=
                    "Bkd KD 0 v=(  ( i(VmeasPU1) * ( i(VmeasIout0) + i(VmeasPC0) + i(VmeasGC0) ) - "
                    "i(VmeasPU0) * ( i(VmeasIout1) + i(VmeasPC1) + i(VmeasGC1) )  )/ ( i(VmeasPD1) "
                    "* "
                    "i(VmeasPU0) - i(VmeasPD0) * i(VmeasPU1)  ) )\n";
            //simul += "Bkd KD 0 v=(1-v(KU))\n";
        }

        else if( !aModel->HasPullup() && aModel->HasPulldown() )
        {
            std::cout << " I have two waveforms, but only one transistor. More equations than "
                         "unknowns."
                      << std::endl;
            simul += "Bku KD 0 v=( ( i(VmeasIout0)+i(VmeasPC0)+i(VmeasGC0) )/(i(VmeasPD0)))\n";
            simul += "Bkd KU 0 v=0\n";
        }

        else if( aModel->HasPullup() && !aModel->HasPulldown() )
        {
            std::cout << " I have two waveforms, but only one transistor. More equations than "
                         "unknowns."
                      << std::endl;
            simul += "Bku KU 0 v=( ( i(VmeasIout)+i(VmeasPC)+i(VmeasGC) )/(i(VmeasPU)))\n";
            simul += "Bkd KD 0 v=0\n";
        }
        else
        {
            std::cout << "ERROR: Driver needs at least a pullup or a pulldown" << std::endl;
        }


        simul += ".tran 0.1n 100n \n";
        simul += ".option xmu=0.49  \n";
        //simul += ".dc Vpin -5 5 0.1\n";
        simul += ".control run \n";
        simul += "set filetype=ascii\n";
        simul += "run \n";
        simul += "plot v(KU) v(KD)\n";
        simul += "write temp_output.spice v(KU) v(KD)\n"; // @TODO we might want to remove this...
        //simul += "quit\n";
        simul += ".endc \n";
        simul += ".end \n";

        getKuKdFromFile( &simul );
    }
    return simul;
}

bool KIBIS_PIN::writeSpiceDriver( std::string* aDest, std::string aName, KIBIS_MODEL* aModel,
                                  IBIS_CORNER aSupply, IBIS_CORNER aSpeed, KIBIS_ACCURACY aAccuracy,
                                  KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType )
{
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
        std::string result;
        std::string tmp;

        result += "\n";
        result = "*Driver model generated by Kicad using Ibis data. ";
        result += "\n.SUBCKT ";
        result += aName;
        result += " PIN GND DIE_M \n"; // 1: POWER, 2:GND, 3:PIN
        result += "\n";

        result += "RPIN 1 PIN ";
        result += doubleToString( R_pin.value[ReverseLogic( aSpeed )] );
        result += "\n";
        result += "LPIN DIE 1 ";
        result += doubleToString( L_pin.value[ReverseLogic( aSpeed )] );
        result += "\n";
        result += "CPIN PIN GND ";
        result += doubleToString( C_pin.value[ReverseLogic( aSpeed )] );
        result += "\n";



        std::vector<std::pair<IbisWaveform*, IbisWaveform*>> wfPairs = aModel->waveformPairs();



        if( wfPairs.size() < 1 || aAccuracy <= KIBIS_ACCURACY::LEVEL_0 )
        {
            if( aAccuracy > KIBIS_ACCURACY::LEVEL_0 )
            {
                std::cout << "Model has no waveform pair, using [Ramp] instead, poor accuracy"
                          << std::endl;
            }
            getKuKdNoWaveform( aModel, aWave, aWaveType, aSupply );
        }
        else if( wfPairs.size() == 1 || aAccuracy <= KIBIS_ACCURACY::LEVEL_1 )
        {
            getKuKdOneWaveform( aModel, wfPairs.at( 0 ), aWave, aWaveType, aSupply, aSpeed );
        }
        else
        {
            if( wfPairs.size() > 2 || aAccuracy <= KIBIS_ACCURACY::LEVEL_2 )
            {
                std::cout << "Model has more than 2 waveform pairs, using the first two."
                          << std::endl;
            }
            getKuKdTwoWaveforms( aModel, wfPairs.at( 0 ), wfPairs.at( 1 ), aWave, aWaveType,
                                 aSupply, aSpeed );
        }

        result += "Vku KU GND pwl ( ";
        for( int i = 0; i < m_t.size(); i++ )
        {
            result += doubleToString( m_t.at( i ) );
            result += " ";
            result += doubleToString( m_Ku.at( i ) );
            result += " ";
        }
        result += ") \n";


        result += "Vkd KD GND pwl ( ";
        for( int i = 0; i < m_t.size(); i++ )
        {
            result += doubleToString( m_t.at( i ) );
            result += " ";
            result += doubleToString( m_Kd.at( i ) );
            result += " ";
        }

        result += ") \n";

        result += aModel->SpiceDie( aSupply, aSpeed );

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


bool KIBIS_PIN::writeSpiceDevice( std::string* aDest, std::string aName, KIBIS_MODEL* aModel,
                                  IBIS_CORNER aSupply, IBIS_CORNER aSpeed )
{
    bool status = true;

    switch( aModel->m_type )
    {
    case IBIS_MODEL_TYPE::INPUT:
    case IBIS_MODEL_TYPE::IO:
    case IBIS_MODEL_TYPE::IO_OPEN_DRAIN:
    case IBIS_MODEL_TYPE::IO_OPEN_SINK:
    case IBIS_MODEL_TYPE::IO_OPEN_SOURCE:
    case IBIS_MODEL_TYPE::IO_ECL:
    {
        std::string result;
        std::string tmp;

        result += "\n";
        result = "*Device model generated by Kicad using Ibis data.";
        result += "\n.SUBCKT ";
        result += aName;
        result += " PIN GND DIE_M \n";
        result += "\n";
        result += "\n";
        result += "RPIN 1 PIN ";
        result += doubleToString( R_pin.value[ReverseLogic( aSpeed )] );
        result += "\n";
        result += "LPIN DIE 1 ";
        result += doubleToString( L_pin.value[ReverseLogic( aSpeed )] );
        result += "\n";
        result += "CPIN PIN GND ";
        result += doubleToString( C_pin.value[ReverseLogic( aSpeed )] );
        result += "\n";


        result += "Vku KU GND pwl ( 0 0 )\n";
        result += "Vkd KD GND pwl ( 0 0 )\n";

        result += aModel->SpiceDie( aSupply, aSpeed );

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