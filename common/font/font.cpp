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
#include <core/wx_stl_compat.h>
#include <kicad_string.h>
#include <font/stroke_font.h>
#include <font/outline_font.h>
#include <trigo.h>
#include <parser/markup_parser.h>
#include <sstream>
#include <eda_text.h>

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
#ifdef DEBUG
    std::cerr << "FONT::getLinePositions( \"" << aText << "\", " << aPosition << ", ..., "
              << aGlyphSize << ", " << aAttributes << " )" << std::endl;
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
    std::cerr << "\"" << aText << "\" line spacing " << aAttributes.GetLineSpacing()
              << " glyph size " << aGlyphSize << " GetInterline() " << GetInterline( aGlyphSize.y )
              << " -> interline " << interline << std::endl;
#endif

    wxPoint                               offset( 0, 0 );
    TEXT_ATTRIBUTES::VERTICAL_ALIGNMENT   vAlignment = aAttributes.GetVerticalAlignment();
    TEXT_ATTRIBUTES::HORIZONTAL_ALIGNMENT hAlignment = aAttributes.GetHorizontalAlignment();
    EDA_ANGLE                             angle = aAttributes.GetAngle();

    if( aAttributes.IsKeepUpright() && aAttributes.GetAngle() > EDA_ANGLE::ANGLE_90 )
    {
        angle = angle.RotateCW().RotateCW();
        hAlignment = TEXT_ATTRIBUTES::OppositeAlignment( hAlignment );
        vAlignment = TEXT_ATTRIBUTES::OppositeAlignment( vAlignment );
    }

    switch( vAlignment )
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

        switch( hAlignment )
        {
        case TEXT_ATTRIBUTES::H_LEFT: break;
        case TEXT_ATTRIBUTES::H_CENTER: lineOffset.x = mirrorX * -textSize.x / 2; break;
        case TEXT_ATTRIBUTES::H_RIGHT: lineOffset.x = mirrorX * -textSize.x;
        }

        wxPoint pos( aPosition.x + lineOffset.x, aPosition.y + lineOffset.y );
        RotatePoint( &pos, origin, angle.AsTenthsOfADegree() );

#ifdef DEBUG
        std::cerr << "Line #" << i << "/" << aLineCount << " pos " << pos << " aPosition "
                  << aPosition << " origin " << origin << " offset " << offset << " lineOffset "
                  << lineOffset << " interline " << interline << " textSize " << textSize
                  << " mirrorX " << mirrorX << " angle " << angle << std::endl;
#endif

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
    aGal->SetHorizontalAlignment( aAttributes.GetHorizontalAlignment() );
    aGal->SetVerticalAlignment( aAttributes.GetVerticalAlignment() );
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
    if( !aGal || aText.empty() )
        return VECTOR2D( 0, 0 );

    EDA_ANGLE angle = aAttributes.GetAngle();
    VECTOR2D  position( aPosition - aOrigin );
#ifdef DEBUG
    std::cerr << "FONT::Draw( aGal, \"" << aText << "\", " << aPosition << ", " << aOrigin << ", "
              << aAttributes << " ) const ; " << this << " position " << position << std::endl;
    bool   drawDebugShapes = false;
    double dbg = 200000;
#endif

#ifdef DEBUG
    KIGFX::COLOR4D oldColor = aGal->GetStrokeColor();
    if( drawDebugShapes )
    {
        aGal->SetStrokeColor( KIGFX::COLOR4D( 1, 1, 0, 1 ) );
        aGal->DrawCircle( aPosition, dbg );
        aGal->SetStrokeColor( KIGFX::COLOR4D( 1, 0, 1, 1 ) );
        aGal->DrawCircle( aOrigin, dbg * 0.8 );
        aGal->SetStrokeColor( KIGFX::COLOR4D( 0, 0, 1, 1 ) );
        aGal->DrawCircle( position, dbg * 1.2 );
        aGal->SetStrokeColor( oldColor );
    }
#endif

    VECTOR2D glyphSize = aAttributes.GetSize();

    // Context needs to be saved before any transformations
    //aGal->Save();

    // Split multiline strings into separate ones and draw them line by line
    wxArrayString         strings_list;
    std::vector<wxPoint>  positions;
    std::vector<VECTOR2D> boundingBoxes;
    int                   n;

    getLinePositions( aText, position, strings_list, positions, n, boundingBoxes, glyphSize,
                      aAttributes );

    VECTOR2D boundingBox( 0, 0 );
    for( int i = 0; i < n; i++ )
    {
#ifdef DEBUG
        std::cerr << "drawing line #" << i << "/" << n << " \"" << strings_list[i] << "\" @"
                  << positions[i].x << "," << positions[i].y << " angle " << angle << std::endl;
#endif
        aGal->Save();
        aGal->Translate( positions[i] );

        if( !angle.IsZero() )
        {
#ifdef DEBUG
            std::cerr << "Nonzero angle " << angle;
#endif
            EDA_ANGLE rotationAngle;
            if( aAttributes.IsKeepUpright() )
            {
#ifdef DEBUG
                std::cerr << "Keeping upright, ";
#endif
                rotationAngle = angle.KeepUpright().Invert();
            }
            else
            {
                rotationAngle = angle.Invert();
            }
#ifdef DEBUG
            std::cerr << "Rotating by " << rotationAngle << std::endl;
#endif
            aGal->Rotate( rotationAngle.AsRadians() );
        }

        VECTOR2D lineBoundingBox = drawSingleLineText( aGal, strings_list[i], VECTOR2D( 0, 0 ) );
        aGal->Restore();

        // expand bounding box of whole text
        boundingBox.x = std::max( boundingBox.x, lineBoundingBox.x );

        double lineHeight = GetInterline( glyphSize.y, aAttributes.GetLineSpacing() );
        boundingBox.y += lineHeight;
    }

    // undo rotation
    //aGal->Restore();

    return boundingBox;
}


VECTOR2D FONT::Draw( KIGFX::GAL* aGal, const EDA_TEXT& aText ) const
{
    VECTOR2D textPos( aText.GetTextPos().x, aText.GetTextPos().y );
    return Draw( aGal, aText.GetShownText(), textPos, aText.GetAttributes() );
}


VECTOR2D FONT::Draw( KIGFX::GAL* aGal, const EDA_TEXT& aText, const VECTOR2D& aPosition ) const
{
    return Draw( aGal, aText.GetShownText(), aPosition, aText.GetAttributes() );
}


VECTOR2D FONT::BoundingBox( const EDA_TEXT& aText )
{
    // TODO: text style flags unconditionally set to 0 - are they even used?
    return getBoundingBox( aText.GetShownText(), aText.GetTextSize(), 0, aText.GetAttributes() );
}


/**
   @return position of cursor for drawing next substring
 */
VECTOR2D FONT::drawMarkup( GLYPH_LIST& aGlyphs, const MARKUP::MARKUP_NODE& aNode,
                           const VECTOR2D& aPosition, const VECTOR2D& aGlyphSize, bool aIsMirrored,
                           const EDA_ANGLE& aAngle, TEXT_STYLE_FLAGS aTextStyle, int aLevel ) const
{
    VECTOR2D nextPosition = aPosition;

    TEXT_STYLE_FLAGS textStyle = aTextStyle;

    if( !aNode->is_root() )
    {
        if( aNode->isSubscript() )
        {
            textStyle = TEXT_STYLE::SUBSCRIPT;
        }
        else if( aNode->isSuperscript() )
        {
            textStyle = TEXT_STYLE::SUPERSCRIPT;
        }

        if( aNode->isOverbar() )
        {
            textStyle |= TEXT_STYLE::OVERBAR;
        }

#ifdef OUTLINEFONT_DEBUG
        std::cerr << "FONT::drawMarkup( [aGlyphs], " << aNode->asString() << ", " << aPosition
                  << ", " << aGlyphSize << ", " << aAngle << ", " << TextStyleAsString( aTextStyle )
                  << ", " << aLevel << " ) const; textStyle " << TextStyleAsString( textStyle )
                  << std::endl;
#endif
        if( aNode->has_content() )
        {
            std::string txt = aNode->string();
            //std::vector<SHAPE_POLY_SET> glyphs;
            wxPoint pt( aPosition.x, aPosition.y );

            nextPosition = GetTextAsPolygon( aGlyphs, txt, aGlyphSize, pt, aAngle, aIsMirrored,
                                             textStyle );
        }
    }

    for( const auto& child : aNode->children )
    {
        nextPosition = drawMarkup( aGlyphs, child, nextPosition, aGlyphSize, aIsMirrored, aAngle,
                                   textStyle, aLevel + 1 );
    }

    return nextPosition;
}


VECTOR2D FONT::drawSingleLineText( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                                   const EDA_ANGLE& aAngle ) const
{
#ifdef DEBUG
    std::cerr << "FONT::drawSingleLineText( aGal, \"" << aText << "\", " << aPosition
              << " )";
#endif

    MARKUP::MARKUP_PARSER markupParser( aText );
    auto                  markupRoot = markupParser.Parse();

    return drawMarkup( aGal, markupRoot, aPosition, aAngle );
}


/**
   @return position of cursor for drawing next substring
 */
VECTOR2D FONT::drawMarkup( KIGFX::GAL* aGal, const MARKUP::MARKUP_NODE& aNode,
                           const VECTOR2D& aPosition, const EDA_ANGLE& aAngle,
                           TEXT_STYLE_FLAGS aTextStyle, int aLevel ) const
{
    if( !aGal )
        return VECTOR2D( 0, 0 );

    VECTOR2D nextPosition = aPosition;

    TEXT_STYLE_FLAGS textStyle = aTextStyle;

    if( !aNode->is_root() )
    {
        if( aNode->isSubscript() )
        {
            textStyle = TEXT_STYLE::SUBSCRIPT;
        }
        else if( aNode->isSuperscript() )
        {
            textStyle = TEXT_STYLE::SUPERSCRIPT;
        }

        if( aNode->isOverbar() )
        {
            textStyle |= TEXT_STYLE::OVERBAR;
        }

        if( aNode->has_content() )
        {
            std::string txt = aNode->string();
            GLYPH_LIST  glyphs;
            wxPoint     pt( aPosition.x, aPosition.y );
            VECTOR2D    glyphSize = aGal->GetGlyphSize();
            bool        mirrored = aGal->IsTextMirrored();

#ifdef DEBUG
            std::cerr << "FONT::drawMarkup( [aGal], " << aNode->asString() << ", "
                      << aPosition << ", " << aAngle << ", " << TextStyleAsString( aTextStyle )
                      << ", " << aLevel << " ) const; txt \"" << txt << "\" pt " << pt.x << ","
                      << pt.y << " glyphSize " << glyphSize << " textStyle {"
                      << TextStyleAsString( textStyle ) << "}" << std::endl;
#endif
            nextPosition =
                    GetTextAsPolygon( glyphs, txt, glyphSize, pt, aAngle, mirrored, textStyle );

            for( auto glyph : glyphs )
                aGal->DrawGlyph( glyph );
        }
    }

    for( const auto& child : aNode->children )
    {
        nextPosition = drawMarkup( aGal, child, nextPosition, aAngle, textStyle, aLevel + 1 );
    }

#ifdef DEBUG
    std::cerr << " returns " << nextPosition << std::endl;
#endif
    return nextPosition;
}
