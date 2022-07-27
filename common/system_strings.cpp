/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <common.h>
#include <string_utils.h>
#include <system_strings.h>


const wxString LAST_UPDATE = wxT( "LAST_UPDATE" );

const std::map<wxString, wxString> dateTimeMap{
    { wxT( "DAY" ), wxT( "%A" ) },   { wxT( "DAY_NBR" ), wxT( "%e" ) },
    { wxT( "MONTH" ), wxT( "%B" ) }, { wxT( "YEAR" ), wxT( "%G" ) },
    { wxT( "HOUR" ), wxT( "%H" ) },  { wxT( "MINUTE" ), wxT( "%M" ) },
    { wxT( "SECOND" ), wxT( "%S" ) }
};


void SYSTEM_STRINGS::SetTextVars( std::map<wxString, wxString>* aTextVars )
{
    m_TextVars = aTextVars;
}


void SYSTEM_STRINGS::UpdateOnSave()
{
    // Update LAST_UPDATE text Var
    Update( LAST_UPDATE, DateAndTime() );

    // Update date and time text Vars
    for( auto dateTextPair : dateTimeMap )
        Update( dateTextPair.first, wxDateTime::Now().Format( dateTextPair.second ) );
}

void SYSTEM_STRINGS::UpdateNow()
{
    Update( wxT( "NOW" ), DateAndTime() );
}

void SYSTEM_STRINGS::Update( const wxString aSystStr, const wxString aWith )
{
    auto textVar = m_TextVars->find( aSystStr );
    if( textVar != m_TextVars->end() )
        textVar->second = aWith;
}