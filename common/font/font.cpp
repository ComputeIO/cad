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

#include <common.h>
#include <kicad_string.h>
#include <font/stroke_font.h>
#include <font/outline_font.h>
#include <trigo.h>
#include <parser/markup_parser.h>
#include <sstream>

std::string TextStyleAsString( TEXT_STYLE_FLAGS aFlags )
{
    std::stringstream s;

    s << "|";
    if( IsBold( aFlags ) )
        s << "BOLD|";
    if( IsItalic( aFlags ) )
        s << "ITALIC|";
    if( IsSuperscript( aFlags ) )
        s << "SUPERSCRIPT|";
    if( IsSubscript( aFlags ) )
        s << "SUBSCRIPT|";
    if( IsOverbar( aFlags ) )
        s << "OVERBAR|";

    return s.str();
}

using namespace KIFONT;

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


wxString FONT::getFontNameForFontconfig( const wxString& aFontName, bool aBold, bool aItalic )
{
    wxString fontName = aFontName;

    if( aBold )
        fontName = fontName.Append( ":bold" );
    if( aItalic )
        fontName = fontName.Append( ":italic" );

    return fontName;
}


FONT* FONT::GetFont( const wxString& aFontName, bool aBold, bool aItalic )
{
#ifdef OUTLINEFONT_DEBUG
    std::cerr << "FONT::GetFont( \"" << aFontName << "\" " << ( aBold ? "bold " : "" )
              << ( aItalic ? "italic " : "" ) << " )\n";
#endif
    if( aFontName.empty() )
        return getDefaultFont();

    wxString fontName = getFontNameForFontconfig( aFontName, aBold, aItalic );

#ifdef OUTLINEFONT_DEBUG
    std::cerr << "looking for \"" << fontName << "\"\n";
#endif
    FONT* font = s_fontMap[fontName];

    if( !font )
    {
        // Try outline first
        font = new OUTLINE_FONT();
        bool success = font->LoadFont( aFontName, aBold, aItalic );

        if( !success )
        {
            // Could not load TrueType, falling back to Hershey
            delete font;
            font = new STROKE_FONT();
            fontName = aFontName;
            success = font->LoadFont( fontName );
            if( !success )
            {
                delete font;
                font = nullptr;
            }
        }

        if( font )
        {
            s_fontMap[fontName] = font;
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
 * @return bounding box width/height
 */
VECTOR2D FONT::doDrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                             bool aParse, const VECTOR2D& aGlyphSize,
                             const TEXT_ATTRIBUTES& aAttributes ) const
{
#ifdef OUTLINEFONT_DEBUG
    std::cerr << "FONT::doDrawString( aGal, \"" << aText << "\", " << aPosition << ", "
              << ", " << ( aParse ? "true" : "false" ) << ", "
              << ", " << aGlyphSize << ", " << aAttributes << " ) const" << std::endl;
#endif
    if( aText.empty() )
        return VECTOR2D( 0.0, 0.0 );

    wxArrayString         strings;
    std::vector<wxPoint>  positions;
    int                   n_lines;
    VECTOR2D              boundingBox;
    std::vector<VECTOR2D> lineBoundingBoxes;

    getLinePositions( aText, aPosition, strings, positions, n_lines, lineBoundingBoxes, aGlyphSize,
                      aAttributes );
#ifdef DEBUG
    std::cerr << "FONT::doDrawString getLinePositions(\"" << aText << "\"): " << n_lines
              << " lines, ";
    for( int i = 0; i < n_lines; i++ )
    {
        std::cerr << "#" << i << " \"" << strings[i] << "\"" << positions[i].x << ","
                  << positions[i].y << " " << lineBoundingBoxes.at( i );
    }
    std::cerr << std::endl;
#endif

    for( int i = 0; i < n_lines; i++ )
    {
#ifdef OUTLINEFONT_DEBUG
        std::cerr << "FONT::doDrawString calling Draw( aGal, \"" << strings.Item( i ) << "\", "
                  << positions[i].x << "," << positions[i].y << ", "
                  << aAttributes.GetAngle().AsDegrees() << " ) i==" << i << std::endl;
#endif
        VECTOR2D lineBoundingBox;
        if( aParse )
        {
            MARKUP::MARKUP_PARSER markupParser( std::string( strings.Item( i ) ) );
            //auto                  parse_result = markupParser.Parse();
            VECTOR2D cursor = positions[i];

            std::function<void( const MARKUP::MARKUP_NODE& )> nodeHandler =
                    [&]( const MARKUP::MARKUP_NODE& aNode )
            {
                if( !aNode->is_root() && aNode->has_content() )
                {
#ifdef OUTLINEFONT_DEBUG
                    std::cerr << "drawing " << aNode->asString() << " at " << cursor << std::endl;
#endif
                    VECTOR2D itemBoundingBox =
                            Draw( aGal, aNode->string(), cursor, aPosition, aAttributes );
                    lineBoundingBox += itemBoundingBox;
                    cursor += itemBoundingBox;
                }

                for( const auto& child : aNode->children )
                    nodeHandler( child );
            };
            nodeHandler( markupParser.Parse() );
        }
        else
        {
            //drawSingleLineText( aGal, strings.Item( i ), positions[i], aRotationAngle );
            lineBoundingBox = Draw( aGal, strings.Item( i ), positions[i], aPosition, aAttributes );
        }

        boundingBox.x = fmax( boundingBox.x, lineBoundingBox.x );
        //boundingBox.y += positions[i].y;
#if 0 //def OUTLINEFONT_DEBUG
        std::cerr << "lineBoundingBox " << lineBoundingBox << " -> boundingBox " << boundingBox
                  << std::endl;
#endif
    }

    boundingBox.y = ( n_lines + 1 ) * GetInterline( aGlyphSize.y );

    return boundingBox;
}


void FONT::getLinePositions( const UTF8& aText, const VECTOR2D& aPosition,
                             wxArrayString& aStringList, std::vector<wxPoint>& aPositions,
                             int& aLineCount, std::vector<VECTOR2D>& aBoundingBoxes,
                             const VECTOR2D& aGlyphSize, const TEXT_ATTRIBUTES& aAttributes ) const
{
#ifdef DEBUG // OUTLINEFONT_DEBUG
    std::cerr << "FONT::getLinePositions( \"" << aText << "\", ..., " << aGlyphSize << ", "
              << aAttributes << " )" << std::endl;
#endif

    wxStringSplit( aText, aStringList, '\n' );
    aLineCount = aStringList.Count();
    aPositions.reserve( aLineCount );

    VECTOR2D maxBoundingBox;

    for( int i = 0; i < aLineCount; i++ )
    {
        VECTOR2D textSize = getBoundingBox( aStringList[i], aGlyphSize );

        if( textSize.x > maxBoundingBox.x )
            maxBoundingBox.x = textSize.x;
        if( textSize.y > maxBoundingBox.y )
            maxBoundingBox.y = textSize.y;

        aBoundingBoxes.push_back( textSize );
    }

    wxPoint origin( aPosition.x, aPosition.y );
    int     interline = GetInterline( aGlyphSize.y, aAttributes.GetLineSpacing() );
#ifdef DEBUG
    std::cerr << "attributes.line_spacing " << aAttributes.GetLineSpacing() << " glyph size "
              << aGlyphSize << " GetInterline() " << GetInterline( aGlyphSize.y )
              << " -> interline " << interline << std::endl;
#endif

    wxPoint offset( 0, 0 );

    switch( aAttributes.GetVerticalAlignment() )
    {
    case TEXT_ATTRIBUTES::V_TOP: offset.y += interline; break;
    case TEXT_ATTRIBUTES::V_CENTER: offset.y -= ( aLineCount - 2 ) * interline / 2; break;
    case TEXT_ATTRIBUTES::V_BOTTOM: offset.y -= ( aLineCount - 1 ) * interline; break;
    }

    int mirrorX = aAttributes.IsMirrored() ? -1 : 1;
    for( int i = 0; i < aLineCount; i++ )
    {
        VECTOR2D textSize = aBoundingBoxes.at( i );
        wxPoint  lineOffset( offset );
        lineOffset.y += i * interline;

        switch( aAttributes.GetHorizontalAlignment() )
        {
        case TEXT_ATTRIBUTES::H_LEFT: break;
        case TEXT_ATTRIBUTES::H_CENTER: lineOffset.x = mirrorX * textSize.x / 2; break;
        case TEXT_ATTRIBUTES::H_RIGHT: lineOffset.x = mirrorX * textSize.x;
        }

        wxPoint pos( aPosition.x + lineOffset.x, aPosition.y + lineOffset.y );
#ifdef DEBUG
        std::cerr << "Line #" << i << "/" << aLineCount << " pos " << pos.x << "," << pos.y
                  << " aPosition " << aPosition.x << "," << aPosition.y << " offset " << offset.x
                  << "," << offset.y << " lineOffset " << lineOffset.x << "," << lineOffset.y
                  << " interline " << interline << " textSize " << textSize << std::endl;
#endif

        // Rotate the position of the line around the origin of the multiline text block
        RotatePoint( &pos, origin, aAttributes.GetAngle().AsTenthsOfADegree() );

        aPositions.push_back( pos );
    }

#ifdef DEBUG
    std::cerr << std::fixed << "FONT::getLinePositions ( \"" << aText << "\", " << aPosition
              << ", ..., " << aGlyphSize << ", " << aAttributes << " ) "
              << " offset " << offset.x << "," << offset.y << " interline " << interline
              << " returns " << aLineCount << " lines:" << std::endl;
    for( int i = 0; i < aLineCount; i++ )
    {
        if( i > 0 )
            std::cerr << ", ";
        std::cerr << "#" << i << "/" << aLineCount << " \"" << aStringList[i] << "\" "
                  << aPositions.at( i ).x << "," << aPositions.at( i ).y << " "
                  << aBoundingBoxes.at( i );
    }
    std::cerr << std::endl;
#endif
}


VECTOR2D FONT::getBoundingBox( const UTF8& aText, const VECTOR2D& aGlyphSize,
                               TEXT_STYLE_FLAGS       aTextStyle,
                               const TEXT_ATTRIBUTES& aAttributes ) const
{
    if( aText.empty() )
        return VECTOR2D( 0.0, 0.0 );

    if( false ) // aParse ) // TODO: parse markup!
    {
        MARKUP::MARKUP_PARSER markupParser( aText );
        auto                  parse_result = markupParser.Parse();

        /* ... */
    }

#ifdef OUTLINEFONT_DEBUG
    std::cerr << "FONT::getBoundingBox( \"" << aText << "\", " << aGlyphSize << ", " << aTextStyle
              << ", " << aAttributes << " )" << std::endl;
#endif
    wxArrayString         strings;
    std::vector<wxPoint>  positions;
    int                   n_lines;
    VECTOR2D              boundingBox;
    std::vector<VECTOR2D> boundingBoxes;

    getLinePositions( aText, VECTOR2D( 0.0, 0.0 ), strings, positions, n_lines, boundingBoxes,
                      aGlyphSize, aAttributes );

    int i = 1;
    for( VECTOR2D lineBoundingBox : boundingBoxes )
    {
#ifdef OUTLINEFONT_DEBUG
        std::cerr << i << "/" << n_lines << ": boundingBox " << boundingBox << " lineBoundingBox "
                  << lineBoundingBox << std::endl;
#endif
        boundingBox.x = fmax( boundingBox.x, lineBoundingBox.x );
        boundingBox.y += lineBoundingBox.y;
        i++;
    }

#ifdef OUTLINEFONT_DEBUG
    std::cerr << "FONT::getBoundingBox( \"" << aText << "\", ... ) returns " << boundingBox
              << std::endl;
#endif
    return boundingBox;
}


void FONT::DrawText( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                     const TEXT_ATTRIBUTES& aAttributes ) const
{
#ifdef DEBUG
    std::cerr << "FONT::DrawText( " << ( aGal ? "[aGal]" : "nullptr" ) << ", \"" << aText << "\", "
              << aPosition << ", " << aAttributes << " )" << std::endl;
#endif
    aGal->SetHorizontalJustify( aAttributes.GetHorizJustify() );
    aGal->SetVerticalJustify( aAttributes.GetVertJustify() );
    aGal->SetGlyphSize( aAttributes.GetSize() );
    aGal->SetFontItalic( aAttributes.IsItalic() );
    aGal->SetFontBold( aAttributes.IsBold() );
    aGal->SetTextMirrored( aAttributes.IsMirrored() );

    Draw( aGal, aText, aPosition, aAttributes );
}


/**
 * Draw a string.
 *
 * @param aGal
 * @param aText is the text to be drawn.
 * @param aPosition is the text position in world coordinates.
 * @param aAngle is the text rotation angle
 */
VECTOR2D FONT::Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                     const VECTOR2D& aOrigin, const TEXT_ATTRIBUTES& aAttributes ) const
{
    if( aText.empty() )
        return VECTOR2D( 0, 0 );

    EDA_ANGLE angle = aAttributes.GetAngle();
    VECTOR2D position( aPosition - aOrigin );
#ifdef DEBUG
    std::cerr << "FONT::Draw( aGal, \"" << aText << "\", " << aPosition << ", " << aOrigin
              << ", " << aAttributes << " ) const ; position " << position << std::endl;
    bool   drawDebugShapes = true;
    double dbg = 200000;
#endif

    if( aGal )
    {
#ifdef DEBUG
        COLOR4D oldColor = aGal->GetStrokeColor();
        if( drawDebugShapes )
        {
            aGal->SetStrokeColor( COLOR4D( 1, 1, 0, 1 ) );
            aGal->DrawCircle( aPosition, dbg );
            aGal->SetStrokeColor( COLOR4D( 1, 0, 1, 1 ) );
            aGal->DrawCircle( aOrigin, dbg * 0.8 );
            aGal->SetStrokeColor( COLOR4D( 0, 0, 1, 1 ) );
            aGal->DrawCircle( position, dbg * 1.2 );
            aGal->SetStrokeColor( oldColor );
        }
#endif
        // Context needs to be saved before any transformations
        aGal->Save();
        //aGal->Translate( aPosition );
        aGal->Rotate( angle.Invert().AsRadians() );
    }

    // Split multiline strings into separate ones and draw them line by line
    wxArrayString         strings_list;
    std::vector<wxPoint>  positions;
    std::vector<VECTOR2D> boundingBoxes;
    int n;

    getLinePositions( aText, position, strings_list, positions, n, boundingBoxes,
                      aGal->GetGlyphSize(), aAttributes );

    VECTOR2D boundingBox( 0, 0 );
    for( int i = 0; i < n; i++ )
    {
#ifdef DEBUG
        std::cerr << "FONT::Draw( aGal, \"" << aText << "\", " << aPosition << ", "
                  << aOrigin << ", " << aAttributes << " ) const "
                  << "drawing line #" << i << "/" << n << " \"" << strings_list[i] << "\" @"
                  << positions[i].x << "," << positions[i].y << std::endl;
#endif
        VECTOR2D lineBoundingBox = drawSingleLineText( aGal, strings_list[i], positions[i] );

        // expand bounding box of whole text
        boundingBox.x = std::max( boundingBox.x, lineBoundingBox.x );

        if( aGal )
        {
            double lineHeight =
                    GetInterline( aGal->GetGlyphSize().y, aAttributes.GetLineSpacing() );
            boundingBox.y += lineHeight;
        }
    }

    if( aGal ) {
        // undo rotation
        aGal->Restore();
    }

    return boundingBox;
}
