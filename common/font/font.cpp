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

#include <font/stroke_font.h>
#include <font/outline_font.h>

FONT*                     FONT::s_defaultFont = nullptr;
std::map<wxString, FONT*> FONT::s_fontMap;

FONT::FONT()
{
}


const wxString& FONT::Name() const
{
    return m_fontName;
}


FONT* FONT::getDefaultFont()
{
    if( !s_defaultFont )
    {
        s_defaultFont = new STROKE_FONT();
        s_defaultFont->LoadFont();
    }

    return s_defaultFont;
}


FONT* FONT::GetFont( const wxString& aFontName )
{
    if( aFontName.empty() )
        return getDefaultFont();

    FONT* font = s_fontMap[aFontName];

    if( !font )
    {
        // Try outline first
        font = new OUTLINE_FONT();
        bool success = font->LoadFont( aFontName );

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

        if( font )
        {
            s_fontMap[aFontName] = font;
        }
        else
        {
            // TODO: signal error? font was not found
            return getDefaultFont();
        }
    }

    return font;
}


bool FONT::IsOutline( const wxString& aFontName )
{
    // note: if aFontName does not name an existing font, default
    // (Newstroke) is used - but IsOutline() returns false for
    // Newstroke anyway, so I guess it's OK
    //
    // TODO: figure out whether it would be a good idea to do
    // something else in case aFontName is not found
    return GetFont( aFontName )->IsOutline();
}
