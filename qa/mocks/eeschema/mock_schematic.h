/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#pragma once
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic.h>

MOCK_BASE_CLASS( MOCK_SCHEMATIC, SCHEMATIC_IFACE )
{
    MOCK_SCHEMATIC() {};
    virtual ~MOCK_SCHEMATIC() {};

    MOCK_CONST_METHOD( ConnectionGraph, 0, CONNECTION_GRAPH*() );
    MOCK_CONST_METHOD( GetSheets, 0, SCH_SHEET_LIST() );
    MOCK_METHOD( SetCurrentSheet, 1, void ( const SCH_SHEET_PATH& aPath ) );
    MOCK_METHOD( CurrentSheet, 0, SCH_SHEET_PATH& () );
    MOCK_CONST_METHOD( GetFileName, 0, wxString () );
    MOCK_CONST_METHOD( Prj, 0, PROJECT& () );
};

