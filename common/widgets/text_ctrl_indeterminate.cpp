/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
* Copyright (C) 2020 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/text_ctrl_indeterminate.h>

#define DEFAULT_INDETERMINATE_STRING "<...>"

TEXT_CTRL_INDETERMINATE::TEXT_CTRL_INDETERMINATE( wxWindow* aParent, wxWindowID aId,
        const wxString& aValue, const wxPoint& aPos, const wxSize& aSize, long aStyle,
        const wxValidator& aValidator, const wxString& aName )
        : wxTextCtrl( aParent, aId, aValue, aPos, aSize, aStyle | wxTE_PROCESS_ENTER, aValidator,
                  aName ),
          m_indeterminateStr( DEFAULT_INDETERMINATE_STRING )
{
    Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( TEXT_CTRL_INDETERMINATE::onTextFocusGet ), NULL,
            this );
    Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TEXT_CTRL_INDETERMINATE::onTextFocusLost ),
            NULL, this );
}


void TEXT_CTRL_INDETERMINATE::SetValue( const wxString& aValue )
{
    if( !checkSetIndeterminateState( aValue ) )
    {
        wxTextCtrl::SetValue( aValue );
    }
}


wxString TEXT_CTRL_INDETERMINATE::GetValue() const
{
    if( IsIndeterminate() )
    {
        return wxEmptyString;
    }

    return wxTextCtrl::GetValue();
}


void TEXT_CTRL_INDETERMINATE::onTextFocusGet( wxFocusEvent& aEvent )
{
    if( IsIndeterminate() )
    {
        wxTextCtrl::SetValue( wxEmptyString );
    }

    aEvent.Skip();
}


void TEXT_CTRL_INDETERMINATE::onTextFocusLost( wxFocusEvent& aEvent )
{
    checkSetIndeterminateState( wxTextCtrl::GetValue() );

    aEvent.Skip();
}


bool TEXT_CTRL_INDETERMINATE::checkSetIndeterminateState( const wxString& aValue )
{
    if( m_canBeIndeterminate && aValue.IsEmpty() )
    {
        wxTextCtrl::SetValue( m_indeterminateStr );
        SetStyle( 0, wxTextCtrl::GetValue().Length(), wxTextAttr( *wxLIGHT_GREY, wxNullColour, *wxITALIC_FONT ) );
        return true;
    }

    return false;
}


bool TEXT_CTRL_INDETERMINATE::IsIndeterminate() const
{
    if( m_canBeIndeterminate && wxTextCtrl::GetValue() == m_indeterminateStr )
    {
        return true;
    }

    return false;
}