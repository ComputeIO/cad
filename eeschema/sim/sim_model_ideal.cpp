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

#include <sim/sim_model_ideal.h>

using PARAM = SIM_MODEL::PARAM;


SIM_MODEL_IDEAL::SIM_MODEL_IDEAL( TYPE aType )
    : SIM_MODEL( aType ),
      m_isInferred( false )
{
    static PARAM::INFO resistor  = makeParamInfo( "r", "Resistance",  "ohm" );
    static PARAM::INFO capacitor = makeParamInfo( "c", "Capacitance", "F"   );
    static PARAM::INFO inductor  = makeParamInfo( "l", "Inductance",  "H"   );

    switch( aType )
    {
    case TYPE::R: AddParam( resistor  ); break;
    case TYPE::C: AddParam( capacitor ); break;
    case TYPE::L: AddParam( inductor  ); break;
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_IDEAL" );
    }
}


void SIM_MODEL_IDEAL::ReadDataSchFields( int aSymbolPinCount,
                                         const std::vector<SCH_FIELD>* aFields )
{
    if( !GetFieldValue( aFields, PARAMS_FIELD ).IsEmpty() )
        SIM_MODEL::ReadDataSchFields( aSymbolPinCount, aFields );
    else
        inferredReadDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL_IDEAL::ReadDataLibFields( int aSymbolPinCount,
                                         const std::vector<LIB_FIELD>* aFields )
{
    if( !GetFieldValue( aFields, PARAMS_FIELD ).IsEmpty() )
        SIM_MODEL::ReadDataLibFields( aSymbolPinCount, aFields );
    else
        inferredReadDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL_IDEAL::WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataSchFields( aFields );

    if( m_isInferred )
        inferredWriteDataFields( aFields );
}


void SIM_MODEL_IDEAL::WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataLibFields( aFields );

    if( m_isInferred )
        inferredWriteDataFields( aFields );
}


wxString SIM_MODEL_IDEAL::GenerateSpiceModelLine( const wxString& aModelName ) const
{
    return "";
}


wxString SIM_MODEL_IDEAL::GenerateSpiceItemLine( const wxString& aRefName,
                                                 const wxString& aModelName,
                                                 const std::vector<wxString>& aPinNetNames ) const
{
    return SIM_MODEL::GenerateSpiceItemLine( aRefName, GetParam( 0 ).value->ToString(),
                                             aPinNetNames );
}


template <typename T>
void SIM_MODEL_IDEAL::inferredReadDataFields( int aSymbolPinCount, const std::vector<T>* aFields )
{
    ParsePinsField( aSymbolPinCount, PINS_FIELD );

    if( ( InferTypeFromRef( GetFieldValue( aFields, REFERENCE_FIELD ) ) == GetType()
            && SetParamValue( 0, GetFieldValue( aFields, VALUE_FIELD ) ) )
        || GetFieldValue( aFields, VALUE_FIELD ) == DeviceTypeInfo( GetDeviceType() ).fieldValue )
    {
        m_isInferred = true;
    }
}


template <typename T>
void SIM_MODEL_IDEAL::inferredWriteDataFields( std::vector<T>& aFields ) const
{
    wxString value = GetParam( 0 ).value->ToString();

    if( value.IsEmpty() )
        value = DeviceTypeInfo( GetDeviceType() ).fieldValue;

    WriteInferredDataFields( aFields, value );
}


PARAM::INFO SIM_MODEL_IDEAL::makeParamInfo( wxString aName, wxString aDescription, wxString aUnit )
{
    SIM_MODEL::PARAM::INFO paramInfo = {};

    paramInfo.name = aName;
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.description = aDescription;

    return paramInfo;
}
