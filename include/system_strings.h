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

#ifndef SYSTEM_STRINGS_H
#define SYSTEM_STRINGS_H

#include <common.h>
#include <richio.h>
#include <map>

/**
 * Variable strings to be used in schematics and boards:
 *   - ${NOW} date/time updated when schematic/board is displayed and saved or printed;
 *   - ${LAST_UPDATE} date/time updated when schematic/board is saved;
 *   - date/time vars to format text :
 *       - ${YEAR} : year
 *       - ${MONTH} : month
 *       - ${DAY_NBR} : day of month (number)
 *       - ${DAY} : day of week (Monday, Tuesday...)
 *       - ${HOUR}, ${MINUTE}, ${SECOND} : hours, minutes, seconds
 *     i.e. : "Today is ${DAY_NBR} ${MONTH} ${YEAR}" = "Today is 07 MAY 2022"
 *
 * These variables should be declared as text Vars in project settings before to be used.
 */
class SYSTEM_STRINGS
{
public:
    void SetTextVars( std::map<wxString, wxString>* aTextVars );
    void UpdateOnSave();
    void UpdateNow();

private:
    std::map<wxString, wxString>* m_TextVars;

    void Update( const wxString aSystStr, const wxString aWith );
};

#endif // SYSTEM_STRINGS_H
