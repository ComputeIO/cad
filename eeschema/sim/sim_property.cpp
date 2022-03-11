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

#include <sim/sim_property.h>
#include <sim/sim_value.h>
#include <ki_exception.h>
#include <confirm.h>
#include <wx/valtext.h>


wxString SIM_VALIDATOR::IsValid( const wxString& aString ) const 
{
    wxString result = wxTextValidator::IsValid( aString );

    if( !result.IsEmpty() )
        return result;

    tao::pegtl::string_input<> in( aString, "from_input" );

    try
    {
        tao::pegtl::parse<SIM_VALUE_PARSER::numberGrammar<SIM_VALUE_PARSER::NOTATION::SI>>( in );
    }
    catch( tao::pegtl::parse_error& e )
    {
        return wxString::Format( _( "Failed to parse '%s': %s" ), aString, e.what() );
    }

    return ""; 
}


bool SIM_VALIDATOR::Validate( wxWindow* aParent )
{
    // Same as wxTextValidator::Validate, except that there is no popup window.

    // If window is disabled, simply return.
    if( !m_validatorWindow->IsEnabled() )
        return true;

    const wxTextEntry* text = GetTextEntry();
    if( !text )
        return false;

    return IsValid( text->GetValue() ).empty();
}


SIM_VALIDATOR::SIM_VALIDATOR() : wxTextValidator( wxFILTER_INCLUDE_CHAR_LIST )
{
}


SIM_INT_VALIDATOR::SIM_INT_VALIDATOR( SIM_VALUE_PARSER::NOTATION aNotation ) : SIM_VALIDATOR()
{
    switch( aNotation )
    {
    case SIM_VALUE_PARSER::NOTATION::SI:
        SetCharIncludes( SIM_VALUE_PARSER::allowedIntChars<SIM_VALUE_PARSER::NOTATION::SI> );
        break;

    case SIM_VALUE_PARSER::NOTATION::SPICE:
        SetCharIncludes( SIM_VALUE_PARSER::allowedFloatChars<SIM_VALUE_PARSER::NOTATION::SPICE> );
        break;
    }
}


SIM_FLOAT_VALIDATOR::SIM_FLOAT_VALIDATOR( SIM_VALUE_PARSER::NOTATION aNotation ) : SIM_VALIDATOR()
{
    switch( aNotation )
    {
    case SIM_VALUE_PARSER::NOTATION::SI:
        SetCharIncludes( SIM_VALUE_PARSER::allowedFloatChars<SIM_VALUE_PARSER::NOTATION::SI> );
        break;

    case SIM_VALUE_PARSER::NOTATION::SPICE:
        SetCharIncludes( SIM_VALUE_PARSER::allowedFloatChars<SIM_VALUE_PARSER::NOTATION::SPICE> );
        break;
    }
}


SIM_PROPERTY::SIM_PROPERTY( const wxString& aLabel, const wxString& aName,
                            SIM_VALUE_BASE& aValue,
                            SIM_VALUE_PARSER::NOTATION aNotation )
    : wxStringProperty( aLabel, aName, aValue.ToString() ),
      m_notation( aNotation ),
      m_value( aValue )
{
}


bool SIM_PROPERTY::StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags ) const
{
    try
    {
        m_value.FromString( aText );
    }
    catch( KI_PARAM_ERROR& e )
    {
        aVariant = aText;
        return false;
    }

    aVariant = m_value.ToString();
    return true;
}


wxValidator* SIM_INT_PROPERTY::DoGetValidator() const
{
    return new SIM_INT_VALIDATOR( m_notation );
}


wxValidator* SIM_FLOAT_PROPERTY::DoGetValidator() const
{
    return new SIM_FLOAT_VALIDATOR( m_notation );
}
