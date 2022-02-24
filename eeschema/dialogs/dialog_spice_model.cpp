/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <widgets/wx_grid.h>
#include <dialog_spice_model.h>
#include <confirm.h>


template class DIALOG_SPICE_MODEL<SCH_FIELD>;
template class DIALOG_SPICE_MODEL<LIB_FIELD>;

template <typename T>
DIALOG_SPICE_MODEL<T>::DIALOG_SPICE_MODEL( wxWindow* aParent, SCH_SYMBOL& aSymbol,
                                           std::vector<T>* aFields )
    : DIALOG_SPICE_MODEL_BASE( aParent ),
      m_symbol( aSymbol ),
      m_fields( aFields ),
      m_firstCategory( nullptr )
{
    try
    {
        SPICE_MODEL::TYPE typeFromFields = SPICE_MODEL::ReadTypeFromFields( aFields );

        for( SPICE_MODEL::TYPE type : SPICE_MODEL::TYPE_ITERATOR() )
        {
            if( type == typeFromFields )
            {
                m_models.emplace_back( aFields );
                m_curModelType = type;
            }
            else
                m_models.emplace_back( type );

            SPICE_MODEL::DEVICE_TYPE deviceType = SPICE_MODEL::TypeInfo( type ).deviceType;

            // By default choose the first model type of each device type.
            if( !m_curModelTypeOfDeviceType.count( deviceType ) )
                m_curModelTypeOfDeviceType[deviceType] = type;
        }
    }
    catch( KI_PARAM_ERROR& e )
    {
        DisplayErrorMessage( this, e.What() );
        return;
    }


    m_typeChoice->Clear();

    for( SPICE_MODEL::DEVICE_TYPE deviceType : SPICE_MODEL::DEVICE_TYPE_ITERATOR() )
        m_deviceTypeChoice->Append( SPICE_MODEL::DeviceTypeInfo( deviceType ).description );

    m_paramGrid = m_paramGridMgr->AddPage();


    m_scintillaTricks = std::make_unique<SCINTILLA_TRICKS>( m_codePreview, wxT( "{}" ), false );

    Layout();
}


template <typename T>
bool DIALOG_SPICE_MODEL<T>::TransferDataFromWindow()
{
    if( !DIALOG_SPICE_MODEL_BASE::TransferDataFromWindow() )
        return false;

    m_models[static_cast<int>( m_curModelType )].WriteFields( m_fields );

    return true;
}


template <typename T>
bool DIALOG_SPICE_MODEL<T>::TransferDataToWindow()
{
    try
    {
        m_models[static_cast<int>( SPICE_MODEL::ReadTypeFromFields( m_fields ) )]
            = SPICE_MODEL( m_fields );
    }
    catch( KI_PARAM_ERROR& e )
    {
        DisplayErrorMessage( this, e.What() );
        return DIALOG_SPICE_MODEL_BASE::TransferDataToWindow();
    }

    updateWidgets();

    return DIALOG_SPICE_MODEL_BASE::TransferDataToWindow();
}


template <typename T>
void DIALOG_SPICE_MODEL<T>::updateWidgets()
{
    SPICE_MODEL::DEVICE_TYPE deviceType = SPICE_MODEL::TypeInfo( m_curModelType ).deviceType;

    m_deviceTypeChoice->SetSelection( static_cast<int>( deviceType ) );
    
    m_typeChoice->Clear();

    for( SPICE_MODEL::TYPE type : SPICE_MODEL::TYPE_ITERATOR() )
    {
        if( SPICE_MODEL::TypeInfo( type ).deviceType == deviceType )
        {
            wxString description = SPICE_MODEL::TypeInfo( type ).description;

            if( !description.IsEmpty() )
                m_typeChoice->Append( description );

            if( type == m_curModelType )
                m_typeChoice->SetSelection( m_typeChoice->GetCount() - 1 );
        }
    }


    // This wxPropertyGridManager stuff has to be here because it crashes in the constructor.

    m_paramGridMgr->SetColumnCount( static_cast<int>( COLUMN::END_ ) );

    m_paramGridMgr->SetColumnTitle( static_cast<int>( COLUMN::UNIT ), "Unit" );
    m_paramGridMgr->SetColumnTitle( static_cast<int>( COLUMN::DEFAULT ), "Default" );
    m_paramGridMgr->SetColumnTitle( static_cast<int>( COLUMN::TYPE ), "Type" );

    m_paramGridMgr->ShowHeader();


    NGSPICE::MODEL_TYPE ngspiceModelType = SPICE_MODEL::TypeInfo( m_curModelType ).ngspiceModelType;
    NGSPICE::MODEL_INFO ngspiceModelInfo = NGSPICE::GetModelInfo( ngspiceModelType );

    m_paramGrid->Clear();

    m_firstCategory = m_paramGrid->Append( new wxPropertyCategory( "Limiting Values" ) );
    m_paramGrid->HideProperty( "Limiting Values" );

    m_paramGrid->Append( new wxPropertyCategory( "DC" ) );
    m_paramGrid->HideProperty( "DC" );

    m_paramGrid->Append( new wxPropertyCategory( "Temperature" ) );
    m_paramGrid->HideProperty( "Temperature" );

    m_paramGrid->Append( new wxPropertyCategory( "Noise" ) );
    m_paramGrid->HideProperty( "Noise" );

    m_paramGrid->Append( new wxPropertyCategory( "Advanced" ) );
    m_paramGrid->HideProperty( "Advanced" );

    m_paramGrid->Append( new wxPropertyCategory( "Flags" ) );
    m_paramGrid->HideProperty( "Flags" );

    for( const auto& [paramName, paramInfo] : ngspiceModelInfo.modelParams )
        addParamPropertyIfRelevant( paramName, paramInfo );

    for( const auto& [paramName, paramInfo] : ngspiceModelInfo.instanceParams )
        addParamPropertyIfRelevant( paramName, paramInfo );

    m_paramGrid->CollapseAll();
}


template <typename T>
void DIALOG_SPICE_MODEL<T>::onDeviceTypeChoice( wxCommandEvent& aEvent )
{
    //SPICE_MODEL::DEVICE_TYPE deviceType = SPICE_MODEL::TypeInfo( m_curModelType ).deviceType;
    SPICE_MODEL::DEVICE_TYPE deviceType =
        static_cast<SPICE_MODEL::DEVICE_TYPE>( m_deviceTypeChoice->GetSelection() );

    m_curModelType = m_curModelTypeOfDeviceType[deviceType];

    updateWidgets();
}


template <typename T>
void DIALOG_SPICE_MODEL<T>::onTypeChoice( wxCommandEvent& aEvent )
{
    SPICE_MODEL::DEVICE_TYPE deviceType =
        static_cast<SPICE_MODEL::DEVICE_TYPE>( m_deviceTypeChoice->GetSelection() );
    wxString typeDescription = m_typeChoice->GetString( m_typeChoice->GetSelection() );

    for( SPICE_MODEL::TYPE type : SPICE_MODEL::TYPE_ITERATOR() )
    {
        if( deviceType == SPICE_MODEL::TypeInfo( type ).deviceType
            && typeDescription == SPICE_MODEL::TypeInfo( type ).description )
        {
            m_curModelType = type;
            break;
        }
    }

    m_curModelTypeOfDeviceType[deviceType] = m_curModelType;
    updateWidgets();
}


//template <typename T>
//void DIALOG_SPICE_MODEL<T>::onGridCellChange( wxGridEvent& aEvent )
//{
    /*updateModel();
    updateWidgets();*/
//}

template <typename T>
void DIALOG_SPICE_MODEL<T>::addParamPropertyIfRelevant( const wxString& paramName,
                                                        const NGSPICE::PARAM_INFO& paramInfo )
{
    if( paramInfo.dir == NGSPICE::PARAM_DIR::OUT || paramInfo.flags.redundant )
        return;

    switch( paramInfo.category )
    {
    case NGSPICE::PARAM_CATEGORY::LIMITING:
        m_paramGrid->HideProperty( "Limiting Values", false );
        m_paramGrid->AppendIn( "Limiting Values", newParamProperty( paramName, paramInfo ) );
        break;

    case NGSPICE::PARAM_CATEGORY::DC:
        m_paramGrid->HideProperty( "DC", false );
        m_paramGrid->AppendIn( "DC", newParamProperty( paramName, paramInfo ) );
        break;

    case NGSPICE::PARAM_CATEGORY::TEMPERATURE:
        m_paramGrid->HideProperty( "Temperature", false );
        m_paramGrid->AppendIn( "Temperature", newParamProperty( paramName, paramInfo ) );
        break;

    case NGSPICE::PARAM_CATEGORY::NOISE:
        m_paramGrid->HideProperty( "Noise", false );
        m_paramGrid->AppendIn( "Noise", newParamProperty( paramName, paramInfo ) );
        break;

    case NGSPICE::PARAM_CATEGORY::ADVANCED:
        m_paramGrid->HideProperty( "Advanced", false );
        m_paramGrid->AppendIn( "Advanced", newParamProperty( paramName, paramInfo ) );
        break;

    case NGSPICE::PARAM_CATEGORY::FLAGS:
        m_paramGrid->HideProperty( "Flags", false );
        m_paramGrid->AppendIn( "Flags", newParamProperty( paramName, paramInfo ) );
        break;

    default:
        //m_paramGrid->AppendIn( nullptr, newParamProperty( paramName, paramInfo ) );
        m_paramGrid->Insert( m_firstCategory, newParamProperty( paramName, paramInfo ) );
        //m_paramGrid->Append( newParamProperty( paramName, paramInfo ) );
        break;

    case NGSPICE::PARAM_CATEGORY::INITIAL_CONDITIONS:
    case NGSPICE::PARAM_CATEGORY::SUPERFLUOUS:
        return;
    }
}

template <typename T>
wxPGProperty* DIALOG_SPICE_MODEL<T>::newParamProperty( const wxString& paramName,
                                                       const NGSPICE::PARAM_INFO& paramInfo ) const
{
    wxString paramDescription = wxString::Format( "%s (%s)", paramInfo.description, paramName );
    wxPGProperty* prop = nullptr;

    switch( paramInfo.type )
    {
    case NGSPICE::PARAM_TYPE::INTEGER: prop = new wxIntProperty( paramDescription );    break;
    case NGSPICE::PARAM_TYPE::REAL:    prop = new wxFloatProperty( paramDescription );  break;

    case NGSPICE::PARAM_TYPE::FLAG:
        prop = new wxBoolProperty( paramDescription );
        prop->SetAttribute( wxPG_BOOL_USE_CHECKBOX, true );
        break;

    default:                           prop = new wxStringProperty( paramDescription ); break;
    }

    prop->SetAttribute( wxPG_ATTR_UNITS, paramInfo.unit );
    prop->SetCell( 3, paramInfo.defaultValueOfVariant1 );

    wxString typeStr;

    switch( paramInfo.type )
    {
    case NGSPICE::PARAM_TYPE::FLAG:      typeStr = wxString( "Bool"            ); break;
    case NGSPICE::PARAM_TYPE::INTEGER:   typeStr = wxString( "Integer"         ); break;
    case NGSPICE::PARAM_TYPE::REAL:      typeStr = wxString( "Float"           ); break;
    case NGSPICE::PARAM_TYPE::COMPLEX:   typeStr = wxString( "Complex"         ); break;
    case NGSPICE::PARAM_TYPE::NODE:      typeStr = wxString( "Node"            ); break;
    case NGSPICE::PARAM_TYPE::INSTANCE:  typeStr = wxString( "Instance"        ); break;
    case NGSPICE::PARAM_TYPE::STRING:    typeStr = wxString( "String"          ); break;
    case NGSPICE::PARAM_TYPE::PARSETREE: typeStr = wxString( "Parsetree"       ); break;
    case NGSPICE::PARAM_TYPE::VECTOR:    typeStr = wxString( "Vector"          ); break;
    case NGSPICE::PARAM_TYPE::FLAGVEC:   typeStr = wxString( "Bool Vector"     ); break;
    case NGSPICE::PARAM_TYPE::INTVEC:    typeStr = wxString( "Int Vector"      ); break;
    case NGSPICE::PARAM_TYPE::REALVEC:   typeStr = wxString( "Float Vector"    ); break;
    case NGSPICE::PARAM_TYPE::CPLXVEC:   typeStr = wxString( "Complex Vector"  ); break;
    case NGSPICE::PARAM_TYPE::NODEVEC:   typeStr = wxString( "Node Vector"     ); break;
    case NGSPICE::PARAM_TYPE::INSTVEC:   typeStr = wxString( "Instance Vector" ); break;
    case NGSPICE::PARAM_TYPE::STRINGVEC: typeStr = wxString( "String Vector"   ); break;
    }

    prop->SetCell( static_cast<int>( COLUMN::TYPE ), typeStr );

    return prop;
}
