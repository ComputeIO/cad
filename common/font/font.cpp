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

#include <kicad_string.h>
#include <font/stroke_font.h>
#include <font/outline_font.h>
#include <trigo.h>

FONT*                     FONT::s_defaultFont = nullptr;
std::map<wxString, FONT*> FONT::s_fontMap;

#ifdef DEBUG
bool FONT::debugMe( const UTF8& aText ) const
{
    return ( !aText.substr( 0, 3 ).compare( "Foo" ) || !aText.substr( 0, 6 ).compare( "${FONT" ) );
}
#endif

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


/**
 * Draw a string.
 *
 * @param aGal
 * @param aTextItem is the underlying text item
 * @param aPosition is the text position
 */
void FONT::DrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                       double aRotationAngle, bool aMultiLine, int aTextWidth, int aTextHeight,
                       EDA_TEXT_HJUSTIFY_T aHorizJustify, EDA_TEXT_VJUSTIFY_T aVertJustify ) const
{
    int textHeight = ( aTextHeight > 0 ? aTextHeight : aGal->GetGlyphSize().y );
#ifdef DEBUG
    if( debugMe( aText ) )
        std::cerr << "FONT::DrawString( aGal, \"" << aText << "\", " << aPosition << ", "
                  << aRotationAngle << ", " << ( aMultiLine ? "true" : "false" ) << ", "
                  << aTextWidth << ", " << aTextHeight << ", " << aVertJustify
                  << " ) const; textHeight " << textHeight << std::endl;
#endif
    if( aText.empty() )
        return;

    wxArrayString        strings_list;
    std::vector<wxPoint> positions;
    int                  n = 1;

    if( aMultiLine )
    {
        wxStringSplit( aText, strings_list, '\n' );
        n = strings_list.Count();
    }
    else
    {
        strings_list.Add( aText );
    }

    positions.reserve( n );

    if( aMultiLine )
    {
#ifdef DEBUG
        if( debugMe( aText ) )
            std::cerr << "getLinePositions( positions, " << n << ", " << aPosition << ", "
                      << textHeight << ", " << aVertJustify << ", " << aRotationAngle
                      << " ) returns ";
#endif
        getLinePositions( positions, n, aPosition, textHeight, aHorizJustify, aVertJustify,
                          aRotationAngle );
#ifdef DEBUG
        if( debugMe( aText ) )
        {
            for( int foo = 0; foo < n; foo++ )
            {
                if( foo > 0 )
                    std::cerr << ", ";
                std::cerr << positions[foo];
            }
            std::cerr << std::endl;
        }
#endif
    }
    else
    {
        wxPoint pt( aPosition.x, aPosition.y );
        positions[0] = pt;
    }

    for( int i = 0; i < n; i++ )
    {
#ifdef DEBUG
        if( debugMe( aText ) )
            std::cerr << "Draw( aGal, \"" << strings_list.Item( i ) << "\", " << positions[i]
                      << ", " << aRotationAngle << " ) i==" << i << std::endl;
#endif
        //drawSingleLineText( aGal, strings_list.Item( i ), positions[i], aRotationAngle );
        Draw( aGal, strings_list.Item( i ), positions[i], aRotationAngle );
    }
}


void FONT::getLinePositions( std::vector<wxPoint>& aPositions, int aLineCount,
                             const VECTOR2D& aPosition, int aTextHeight,
                             EDA_TEXT_HJUSTIFY_T aHorizJustify, EDA_TEXT_VJUSTIFY_T aVertJustify,
                             double aRotationAngle ) const
{
    // Position of first line of the multiline text according to the
    // center of the multiline text block
    wxPoint pos( aPosition.x, aPosition.y );
    // Offset to next line.
    wxPoint offset;

    offset.y = GetInterline( aTextHeight ? aTextHeight : 1 );

    //VECTOR2D textSize = ComputeTextLineSize( aGal, a

    if( aLineCount > 1 )
    {
        switch( aVertJustify )
        {
        case GR_TEXT_VJUSTIFY_TOP: break;

        case GR_TEXT_VJUSTIFY_CENTER: pos.y -= ( aLineCount - 1 ) * offset.y / 2; break;

        case GR_TEXT_VJUSTIFY_BOTTOM: pos.y -= ( aLineCount - 1 ) * offset.y; break;
        }
    }

    // Rotate the position of the first line
    // around the center of the multiline text block
    wxPoint origin( aPosition.x, aPosition.y );
    RotatePoint( &pos, origin, aRotationAngle );

    // Rotate the offset lines to increase happened in the right direction
    RotatePoint( &offset, aRotationAngle );

    for( int i = 0; i < aLineCount; i++ )
    {
        aPositions.push_back( pos );
        pos += offset;
    }
}
