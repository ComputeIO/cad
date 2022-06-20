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
#include <wx/combo.h>
#include <wx/combobox.h>


wxBEGIN_EVENT_TABLE( SIM_VALIDATOR, wxValidator )
    EVT_TEXT( wxID_ANY, SIM_VALIDATOR::onText )
wxEND_EVENT_TABLE()


SIM_VALIDATOR::SIM_VALIDATOR( SIM_VALUE_BASE::TYPE aValueType,
                              SIM_VALUE_PARSER::NOTATION aNotation )
    : wxValidator(),
      m_valueType( aValueType ),
      m_notation( aNotation )
{
    wxTextEntry* textEntry = getTextEntry();
    if( !textEntry )
        return;

    m_prevText = textEntry->GetValue();
    m_prevInsertionPoint = textEntry->GetInsertionPoint();
}


wxObject* SIM_VALIDATOR::Clone() const
{
    return new SIM_VALIDATOR( *this );
}


bool SIM_VALIDATOR::Validate( wxWindow* aParent )
{
    if( !m_validatorWindow->IsEnabled() )
        return true;

    wxTextEntry* const textEntry = getTextEntry();
    if( !textEntry )
        return false;

    return isValid( textEntry->GetValue() );
}


bool SIM_VALIDATOR::TransferToWindow()
{
    return true;
}


bool SIM_VALIDATOR::TransferFromWindow()
{
    return true;
}


bool SIM_VALIDATOR::isValid( const wxString& aString )
{
    return SIM_VALUE_PARSER::IsValid( aString, m_valueType, m_notation );
}


void SIM_VALIDATOR::onText( wxCommandEvent& aEvent )
{
    wxTextEntry* textEntry = getTextEntry();
    if( !textEntry )
        return;

    if( !isValid( textEntry->GetValue() ) )
    {
        textEntry->ChangeValue( m_prevText );
        textEntry->SetInsertionPoint( m_prevInsertionPoint );
    }

    m_prevText = textEntry->GetValue();
    m_prevInsertionPoint = textEntry->GetInsertionPoint();
}


wxTextEntry* SIM_VALIDATOR::getTextEntry()
{
    // Taken from wxTextValidator.

    if( wxDynamicCast( m_validatorWindow, wxTextCtrl ) )
        return ( wxTextCtrl* ) m_validatorWindow;

    if( wxDynamicCast( m_validatorWindow, wxComboBox ) )
        return ( wxComboBox* ) m_validatorWindow;

    if( wxDynamicCast( m_validatorWindow, wxComboCtrl ) )
        return ( wxComboCtrl* ) m_validatorWindow;

    wxFAIL_MSG(
        "SIM_VALIDATOR can only be used with wxTextCtrl, wxComboBox, or wxComboCtrl"
    );

    return nullptr;
}


SIM_PROPERTY::SIM_PROPERTY( const wxString& aLabel, const wxString& aName,
                            SIM_VALUE_BASE& aValue,
                            SIM_VALUE_BASE::TYPE aValueType,
                            SIM_VALUE_PARSER::NOTATION aNotation )
    : wxStringProperty( aLabel, aName, aValue.ToString() ),
      m_valueType( aValueType ),
      m_notation( aNotation ),
      m_value( aValue )
{
}


wxValidator* SIM_PROPERTY::DoGetValidator() const
{
    return new SIM_VALIDATOR( m_valueType, m_notation );
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
