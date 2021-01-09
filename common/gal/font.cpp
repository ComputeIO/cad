/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Font abstract base class
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

#include <gal/stroke_font.h>
#include <gal/truetype_font.h>

using namespace KIGFX;

FONT* FONT::defaultFont = nullptr;

FONT::FONT()
{
}


FONT* FONT::GetFont( const wxString& aFontName )
{
    if( aFontName.empty() )
    {
        if( !defaultFont )
        {
            defaultFont = new STROKE_FONT();
            defaultFont->LoadFont(); // TODO: do something on failure
        }

        return defaultFont;
    }

    // Try TrueType first
    FONT* font = new TRUETYPE_FONT();
    bool  success = font->LoadFont( aFontName );

    if( !success )
    {
        // Could not load TrueType, falling back to Hershey
        delete font;
        font = new STROKE_FONT();
        success = font->LoadFont( aFontName );
        if( !success )
        {
            delete font;
            font = nullptr;
        }
    }

    return font;
}
