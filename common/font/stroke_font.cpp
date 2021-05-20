/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2016 Kicad Developers, see change_log.txt for contributors.
 *
 * Stroke font class
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

#include <gal/graphics_abstraction_layer.h>
#include <math/util.h> // for KiROUND
#include <wx/string.h>
#include <wx/textfile.h>
#include <gr_text.h>
#include <settings/settings_manager.h>
#include <newstroke_font.h>
#include <font/stroke_font.h>
#include <parser/markup_parser.h>
#include <cmath>
#include <kicad_string.h>
#include <geometry/shape_line_chain.h>
#include <trigo.h>

using namespace KIFONT;

GLYPH_LIST*         g_defaultFontGlyphs = nullptr;   ///< Glyph list
std::vector<BOX2D>* g_defaultFontGlyphBoundingBoxes; ///< Bounding boxes of the glyphs


STROKE_FONT::STROKE_FONT() : m_glyphs( nullptr ), m_glyphBoundingBoxes( nullptr )
{
}


bool STROKE_FONT::LoadFont( const wxString& aFontName, bool aBold, bool aItalic )
{
    if( aFontName.empty() )
    {
        m_fontName = "";
        return loadNewStrokeFont( newstroke_font, newstroke_font_bufsize );
    }
    else
    {
        bool success = loadHersheyFont( aFontName );

        if( success )
        {
            m_fontName = aFontName;
        }

        return success;
    }
}

bool STROKE_FONT::loadNewStrokeFont( const char* const aNewStrokeFont[], int aNewStrokeFontSize )
{
    if( g_defaultFontGlyphs )
    {
        m_glyphs = g_defaultFontGlyphs;
        m_glyphBoundingBoxes = g_defaultFontGlyphBoundingBoxes;
        return true;
    }

    g_defaultFontGlyphs = new GLYPH_LIST;
    g_defaultFontGlyphs->reserve( aNewStrokeFontSize );

    g_defaultFontGlyphBoundingBoxes = new std::vector<BOX2D>;
    g_defaultFontGlyphBoundingBoxes->reserve( aNewStrokeFontSize );

    for( int j = 0; j < aNewStrokeFontSize; j++ )
    {
        GLYPH* glyph = new GLYPH;
        double glyphStartX = 0.0;
        double glyphEndX = 0.0;
        double glyphWidth = 0.0;

        std::vector<VECTOR2D>* pointList = nullptr;

        int strokes = 0;
        int i = 0;

        while( aNewStrokeFont[j][i] )
        {
            if( aNewStrokeFont[j][i] == ' ' && aNewStrokeFont[j][i + 1] == 'R' )
                strokes++;

            i += 2;
        }

        glyph->reserve( strokes + 1 );

        i = 0;

        while( aNewStrokeFont[j][i] )
        {
            VECTOR2D point( 0.0, 0.0 );
            char     coordinate[2] = {
                0,
            };

            for( int k : { 0, 1 } )
                coordinate[k] = aNewStrokeFont[j][i + k];

            if( i < 2 )
            {
                // The first two values contain the width of the char
                glyphStartX = ( coordinate[0] - 'R' ) * STROKE_FONT_SCALE;
                glyphEndX = ( coordinate[1] - 'R' ) * STROKE_FONT_SCALE;
                glyphWidth = glyphEndX - glyphStartX;
            }
            else if( ( coordinate[0] == ' ' ) && ( coordinate[1] == 'R' ) )
            {
                if( pointList )
                    pointList->shrink_to_fit();

                // Raise pen
                pointList = nullptr;
            }
            else
            {
                // In stroke font, coordinates values are coded as <value> + 'R',
                // <value> is an ASCII char.
                // therefore every coordinate description of the Hershey format has an offset,
                // it has to be subtracted
                // Note:
                //  * the stroke coordinates are stored in reduced form (-1.0 to +1.0),
                //    and the actual size is stroke coordinate * glyph size
                //  * a few shapes have a height slightly bigger than 1.0 ( like '{' '[' )
                point.x = (double) ( coordinate[0] - 'R' ) * STROKE_FONT_SCALE - glyphStartX;

                // FONT_OFFSET is here for historical reasons, due to the way the stroke font
                // was built. It allows shapes coordinates like W M ... to be >= 0
                // Only shapes like j y have coordinates < 0
                point.y = (double) ( coordinate[1] - 'R' + FONT_OFFSET ) * STROKE_FONT_SCALE;

                if( !pointList )
                {
                    pointList = new std::vector<VECTOR2D>;
                    glyph->push_back( pointList );
                }

                pointList->push_back( point );
            }

            i += 2;
        }

        if( pointList )
            pointList->shrink_to_fit();

        // Compute the bounding box of the glyph
        g_defaultFontGlyphBoundingBoxes->emplace_back( computeBoundingBox( glyph, glyphWidth ) );
        g_defaultFontGlyphs->push_back( glyph );
        m_maxGlyphWidth =
                std::max( m_maxGlyphWidth, g_defaultFontGlyphBoundingBoxes->back().GetWidth() );
    }

    m_glyphs = g_defaultFontGlyphs;
    m_glyphBoundingBoxes = g_defaultFontGlyphBoundingBoxes;
    return true;
}


GLYPH* STROKE_FONT::processGlyph( std::string aGlyphString, double& aGlyphWidth )
{
    // handle glyph data
    GLYPH*                 glyph = new GLYPH;
    double                 glyphStartX = 0.0;
    double                 glyphEndX = 0.0;
    int                    strokes = 0;
    std::vector<VECTOR2D>* pointList = nullptr;
    int                    i;

    for( i = 5; i < 8; i++ )
    {
        if( aGlyphString[i] > '0' && aGlyphString[i] <= '9' )
        {
            int n = aGlyphString[i] - '0';
            if( i == 5 )
            {
                strokes += n * 100;
            }
            else if( i == 6 )
            {
                strokes += n * 10;
            }
            else
            {
                strokes += n;
            }
        }
    }

    glyph->reserve( strokes + 1 );

    i = 8;

    while( aGlyphString[i] )
    {
        VECTOR2D point( 0.0, 0.0 );
        char     coordinate[2] = {
            0,
        };

        for( int k : { 0, 1 } )
            coordinate[k] = aGlyphString[i + k];

        if( i < 10 )
        {
            // The first two values contain the width of the char
            glyphStartX = ( coordinate[0] - 'R' ) * STROKE_FONT_SCALE;
            glyphEndX = ( coordinate[1] - 'R' ) * STROKE_FONT_SCALE;
            aGlyphWidth = glyphEndX - glyphStartX;
        }
        else if( ( coordinate[0] == ' ' ) && ( coordinate[1] == 'R' ) )
        {
            if( pointList )
                pointList->shrink_to_fit();

            // Raise pen
            pointList = nullptr;
        }
        else
        {
            // In stroke font, coordinates values are coded as <value> + 'R',
            // <value> is an ASCII char.
            // therefore every coordinate description of the Hershey format has an offset,
            // it has to be subtracted
            // Note:
            //  * the stroke coordinates are stored in reduced form (-1.0 to +1.0),
            //    and the actual size is stroke coordinate * glyph size
            //  * a few shapes have a height slightly bigger than 1.0 ( like '{' '[' )
            point.x = (double) ( coordinate[0] - 'R' ) * STROKE_FONT_SCALE - glyphStartX;

            // FONT_OFFSET is here for historical reasons, due to the way the stroke font
            // was built. It allows shapes coordinates like W M ... to be >= 0
            // Only shapes like j y have coordinates < 0
            point.y = (double) ( coordinate[1] - 'R' + FONT_OFFSET ) * STROKE_FONT_SCALE;

            if( !pointList )
            {
                pointList = new std::vector<VECTOR2D>;
                glyph->push_back( pointList );
            }

            pointList->push_back( point );
        }

        i += 2;
    }

    if( pointList )
        pointList->shrink_to_fit();

    return glyph;
}


bool STROKE_FONT::loadHersheyFont( const wxString& aFontName )
{
    // TODO combine with loadNewStrokeFont()

    if( aFontName.IsEmpty() )
    {
        return false;
    }

    wxFileName fontFile( aFontName );
    fontFile.SetExt( "jhf" );
    fontFile.SetPath( SETTINGS_MANAGER::GetUserSettingsPath() + wxT( "/fonts" ) );

    wxString fileName = fontFile.GetFullPath();

    if( !wxFile::Exists( fileName ) )
    {
        return false;
    }

    wxTextFile font;

    if( !font.Open( fileName ) )
    {
        return false;
    }

    GLYPH_LIST*              fontGlyphs = new GLYPH_LIST;
    GLYPH_BOUNDING_BOX_LIST* fontGlyphBoundingBoxes = new GLYPH_BOUNDING_BOX_LIST;

    int         n_glyph = 0;
    std::string glyphString;

    for( auto glyphData = font.GetFirstLine(); !font.Eof(); glyphData = font.GetNextLine() )
    {
        wxCharBuffer cb = glyphData.ToAscii();
        const char*  cbData = cb.data();
        bool         continuationLine = false;
        int          i;

        for( i = 0; i < 5; i++ )
        {
            // if there's something other than a space or digit in the first 5 columns,
            // this line is a continuation of the previous line
            if( !( cbData[i] == ' ' || ( cbData[i] >= '0' && cbData[i] <= '9' ) ) )
            {
                continuationLine = true;
                break;
            }
        }

        double glyphWidth = 0.0;
        GLYPH* glyph;

        if( !continuationLine && !glyphString.empty() )
        {
            // Process current glyphString contents first
            glyph = processGlyph( glyphString, glyphWidth );

            // Compute the bounding box of the glyph
            fontGlyphBoundingBoxes->emplace_back( computeBoundingBox( glyph, glyphWidth ) );
            fontGlyphs->push_back( glyph );

            // Done, get ready for next glyph
            glyphString.clear();
            n_glyph++;
        }

        // TODO: make sure glyphString does not expand to fill all available memory
        glyphString.append( cbData );

        if( font.Eof() )
        {
            // Process last glyph
            glyph = processGlyph( glyphString, glyphWidth );

            // Compute the bounding box of the glyph
            fontGlyphBoundingBoxes->emplace_back( computeBoundingBox( glyph, glyphWidth ) );
            fontGlyphs->push_back( glyph );
        }
    }

    m_fontName = aFontName;
    m_glyphs = fontGlyphs;
    m_glyphBoundingBoxes = fontGlyphBoundingBoxes;
    return true;
}


double STROKE_FONT::GetInterline( double aGlyphHeight, double aLineSpacing ) const
{
    // Do not add the glyph thickness to the interline.  This makes bold text line-spacing
    // different from normal text, which is poor typography.
    return ( aGlyphHeight * aLineSpacing * INTERLINE_PITCH_RATIO );
}


BOX2D STROKE_FONT::computeBoundingBox( const GLYPH* aGLYPH, double aGlyphWidth ) const
{
    VECTOR2D min( 0, 0 );
    VECTOR2D max( aGlyphWidth, 0 );

    for( const std::vector<VECTOR2D>* pointList : *aGLYPH )
    {
        for( const VECTOR2D& point : *pointList )
        {
            min.y = std::min( min.y, point.y );
            max.y = std::max( max.y, point.y );
        }
    }

    return BOX2D( min, max - min );
}


VECTOR2D STROKE_FONT::drawSingleLineText( KIGFX::GAL* aGal, const UTF8& aText,
                                          const VECTOR2D& aPosition, const EDA_ANGLE& aAngle ) const
{
#ifdef DEBUG
    std::cerr << "STROKE_FONT::drawSingleLineText( [aGal], " << aText << ", " << aPosition << ", "
              << aAngle << " ) "
              << "aGal line width " << ( aGal ? aGal->GetLineWidth() : 0.0f ) << std::endl;
    bool   drawDebugShapes = false;
    double debugLineWidth = 1500.0;
#endif
    if( !aGal || aText.empty() )
        return VECTOR2D( 0, 0 );

    VECTOR2D baseGlyphSize( aGal->GetGlyphSize() );
    bool     underlined = aGal->IsFontUnderlined();
    bool     italic = aGal->IsFontItalic();
    bool     mirrored = aGal->IsTextMirrored();
    double   italic_tilt = italic ? ( mirrored ? -ITALIC_TILT : ITALIC_TILT ) : 0;
    double   overbar_italic_comp = computeOverbarVerticalPosition( aGal ) * italic_tilt;
    VECTOR2D textSize = ComputeTextLineSize( aGal, aText );
    double   half_thickness = aGal->GetLineWidth() / 2;
    double   xOffset = 0.0;
    double   yOffset = 0.0;

    if( mirrored )
    {
        // In case of mirrored text invert the X scale of points and their X direction
        // (m_glyphSize.x) and start drawing from the position where text normally should end
        // (textSize.x)
        xOffset = textSize.x - aGal->GetLineWidth();
        baseGlyphSize.x = -baseGlyphSize.x;
    }

#ifdef DEBUG
    std::cerr << "STROKE_FONT::drawSingleLineText( [aGal], \"" << aText << "\", " << aPosition
              << ", " << aAngle << " )\nbaseGlyphSize " << baseGlyphSize << " textSize " << textSize
              << " half_thickness " << half_thickness << " overbar_italic_comp "
              << overbar_italic_comp << ( italic ? " " : " !" ) << "italic "
              << ( underlined ? "" : "!" ) << "underlined " << ( mirrored ? "" : "!" )
              << "mirrored xOffset " << xOffset << " aGal{line width " << aGal->GetLineWidth()
              << " hAlign " << aGal->GetHorizontalAlignment() << " vAlign "
              << aGal->GetVerticalAlignment() << " zoom " << aGal->GetZoomFactor() << " scale "
              << aGal->GetWorldScale() << "}" << std::endl;
    int dbg = 40000;

    if( drawDebugShapes )
    {
        VECTOR2D debugPosition( aPosition );
        VECTOR2D offsetPosition( aPosition.x + xOffset, aPosition.y + yOffset );
        KIGFX::COLOR4D  oldColor = aGal->GetStrokeColor();
        double   lw = aGal->GetLineWidth();
        aGal->SetLineWidth( debugLineWidth );
        aGal->SetStrokeColor( KIGFX::COLOR4D( .7, .7, .7, .15 ) );
        aGal->DrawCircle( offsetPosition, 0.75 * dbg );
        //
        double hd = dbg / 4;
        aGal->SetStrokeColor( KIGFX::COLOR4D( .8, .8, .8, .1 ) );
        // crosshair
        aGal->DrawLine( VECTOR2D( aPosition.x, aPosition.y - hd ),
                        VECTOR2D( aPosition.x, aPosition.y + hd ) );
        aGal->DrawLine( VECTOR2D( aPosition.x - hd, aPosition.y ),
                        VECTOR2D( aPosition.x + hd, aPosition.y ) );
        aGal->SetLineWidth( lw );
        aGal->SetStrokeColor( oldColor );
    }
#endif

    // First adjust: the text X position is corrected by half_thickness
    // because when the text with thickness is draw, its full size is textSize,
    // but the position of lines is half_thickness to textSize - half_thickness
    // so we must translate the coordinates by half_thickness on the X axis
    // to place the text inside the 0 to textSize X area.
    //
    double xAdjust = half_thickness;

    switch( aGal->GetHorizontalAlignment() )
    {
    case TEXT_ATTRIBUTES::H_LEFT: break;
    case TEXT_ATTRIBUTES::H_RIGHT: xAdjust += -textSize.x; break;
    case TEXT_ATTRIBUTES::H_CENTER:
    default: xAdjust += -textSize.x / 2.0;
    }

    double lineHeight = textSize.y - GetInterline( baseGlyphSize.y );
    double yAdjust = 0.0;
    switch( aGal->GetVerticalAlignment() )
    {
    case TEXT_ATTRIBUTES::V_TOP: yAdjust = lineHeight / 2; break;
    case TEXT_ATTRIBUTES::V_BOTTOM: yAdjust = -lineHeight / 2; break;
    case TEXT_ATTRIBUTES::V_CENTER:
    default: break;
    }

#ifdef SHOULD_THIS_BE_DONE_AFTER_ALL
    // there's a weird 1/4 line height offset coming from somewhere;
    // let's compensate it - TODO: find out where it comes from
    yAdjust += GetInterline( baseGlyphSize.y ) / 4;
#endif

    VECTOR2D hv( xAdjust, yAdjust );
#ifdef DEBUG
    std::cerr << "Adjusting by " << hv << std::endl;
    if( false ) //drawDebugShapes )
    {
        std::cerr << "Getting line width, ";
        double lw = aGal->GetLineWidth();
        std::cerr << "Line width is " << lw << std::endl;
        aGal->SetLineWidth( debugLineWidth * 1.1 );
        std::cerr << "Drawing rectangle " << aPosition << ", " << hv << std::endl;
        aGal->DrawRectangle( aPosition, hv );
        VECTOR2D ts( aPosition.x + ( -textSize.x / 2 ), aPosition.y + ( -textSize.y / 2 ) );
        std::cerr << "Drawing rectangle " << ts << ", " << textSize << std::endl;
        aGal->DrawRectangle( ts, textSize );
        aGal->SetLineWidth( lw );
    }
    std::cerr << "Parsing markup next" << hv << std::endl;
#endif

    MARKUP::MARKUP_PARSER markup_parser( aText );
    markup_parser.Parse();

    // The overbar is indented inward at the beginning of an italicized section, but
    // must not be indented on subsequent letters to ensure that the bar segments
    // overlap.
    bool     last_had_overbar = false;
    bool     in_overbar = false;
    bool     in_super_or_subscript = false;
    VECTOR2D glyphSize = baseGlyphSize;
    int      char_count = 0;

    std::vector<SHAPE_LINE_CHAIN> glyphParts;
    SHAPE_LINE_CHAIN              charGlyph;

    for( UTF8::uni_iter chIt = aText.ubegin(), end = aText.uend(); chIt < end; ++chIt )
    {
        // Handle tabs as locked to the nearest 4th column (counting in spaces)
        // The choice of spaces is somewhat arbitrary but sufficient for aligning text
        if( *chIt == '\t' )
        {
            // We align to the 4th column.  This is based on the monospace font used in the text input
            // boxes.  Here, we take the widest character as our baseline spacing and make tab stops
            // at each fourth of this widest character
            char_count = ( char_count / 4 + 1 ) * 4 - 1;
            xOffset = m_maxGlyphWidth * baseGlyphSize.x * char_count;

            glyphSize = baseGlyphSize;
            yOffset = 0;

            // Tab ends an overbar
            in_overbar = false;
        }
        else if( *chIt == '~' )
        {
            if( ++chIt == end )
                break;

            if( *chIt == '~' )
            {
                // double ~ is really a ~ so go ahead and process the second one

                // so what's a triple ~?  It could be a real ~ followed by an overbar, or
                // it could be an overbar followed by a real ~.  The old algorithm did the
                // former so we will too....
            }
            else
            {
                in_overbar = !in_overbar;
            }
        }
        else if( *chIt == '^' )
        {
            auto lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                //  process superscript
                chIt = lookahead;
                in_super_or_subscript = true;
                glyphSize = baseGlyphSize * 0.8;
                yOffset = -baseGlyphSize.y * 0.3;
                continue;
            }
        }
        else if( *chIt == '_' )
        {
            auto lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                //  process subscript
                chIt = lookahead;
                in_super_or_subscript = true;
                glyphSize = baseGlyphSize * 0.8;
                yOffset = baseGlyphSize.y * 0.1;
                continue;
            }
        }
        else if( *chIt == '}' && in_super_or_subscript )
        {
            in_super_or_subscript = false;
            glyphSize = baseGlyphSize;
            yOffset = 0;
            continue;
        }
        // Overbar syntax is less precise so we have to have some special cases
        else if( in_overbar && ( *chIt == ' ' || *chIt == '}' || *chIt == ')' ) )
        {
            in_overbar = false;
        }

        // Index into bounding boxes table
        int dd = (signed) *chIt - ' ';

        if( dd >= (int) m_glyphBoundingBoxes->size() || dd < 0 )
        {
            int substitute = *chIt == '\t' ? ' ' : '?';
            dd = substitute - ' ';
        }

        const GLYPH* glyph = m_glyphs->at( dd );
        const BOX2D& bbox = m_glyphBoundingBoxes->at( dd );

        if( in_overbar )
        {
            double overbar_start_x = xOffset;
            double overbar_start_y = -computeOverbarVerticalPosition( aGal );
            double overbar_end_x = xOffset + glyphSize.x * bbox.GetEnd().x;
            double overbar_end_y = overbar_start_y;

            if( !last_had_overbar )
            {
                if( italic )
                    overbar_start_x += overbar_italic_comp;

                last_had_overbar = true;
            }

            VECTOR2D startOverbar( overbar_start_x, overbar_start_y );
            VECTOR2D endOverbar( overbar_end_x, overbar_end_y );

            aGal->DrawLine( startOverbar, endOverbar );
        }
        else
        {
            last_had_overbar = false;
        }

        if( underlined )
        {
            double   vOffset = computeUnderlineVerticalPosition( aGal );
            VECTOR2D startUnderline( xOffset, -vOffset );
            VECTOR2D endUnderline( xOffset + glyphSize.x * bbox.GetEnd().x, -vOffset );

            aGal->DrawLine( startUnderline, endUnderline );
        }

        VECTOR2D pos( aPosition.x + xOffset, aPosition.y + yOffset );
        for( const std::vector<VECTOR2D>* ptList : *glyph )
        {
            charGlyph.Clear();

            for( const VECTOR2D& pt : *ptList )
            {
                VECTOR2D point( pt );
                if( !aAngle.AsDegrees() == 0 )
                {
#ifdef DEBUG
                    std::cerr << "Rotating point, angle is " << aAngle << std::endl;
#endif
                    // Rotate point
                    RotatePoint( &point.x, &point.y, 0, 0,
                                 (double) aAngle.AsTenthsOfADegree() );
                }

                double x = point.x * glyphSize.x + pos.x;
                double y = point.y * glyphSize.y + pos.y;
                x += -y * italic_tilt;
                point = VECTOR2D( x, y );

                charGlyph.Append( point );
            }

            charGlyph.SetClosed( false );
            glyphParts.push_back( charGlyph );
        }

        char_count++;
        xOffset += glyphSize.x * bbox.GetEnd().x;
    }

    for( const SHAPE_LINE_CHAIN& g : glyphParts )
    {
        if( g.PointCount() >= 2 )
            aGal->DrawPolyline( g );
    }

    return VECTOR2D( xOffset, yOffset );
}


double STROKE_FONT::ComputeOverbarVerticalPosition( double aGlyphHeight ) const
{
    // Static method.
    return aGlyphHeight * OVERBAR_POSITION_FACTOR;
}


double STROKE_FONT::computeOverbarVerticalPosition( const KIGFX::GAL* aGal ) const
{
    // Compute the Y position of the overbar. This is the distance between
    // the text base line and the overbar axis.
    // TODO: default 16.0 is a guess
    return ComputeOverbarVerticalPosition( aGal ? aGal->GetGlyphSize().y : 16.0 );
}


double STROKE_FONT::computeUnderlineVerticalPosition( const KIGFX::GAL* aGal ) const
{
    // Compute the Y position of the underline. This is the distance between
    // the text base line and the underline axis.
    // TODO: default 16.0 is a guess
    return -1 * ( aGal ? aGal->GetGlyphSize().y : 16.0 ) * UNDERLINE_POSITION_FACTOR;
}


VECTOR2D STROKE_FONT::ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const
{
    //TODO default glyph size (and line width) is a guess
    VECTOR2D glyphSize = aGal ? aGal->GetGlyphSize() : VECTOR2D( 16.0, 16.0 );
    double   lineWidth = aGal ? aGal->GetLineWidth() : 1.0;
    return StringBoundaryLimits( aGal, aText, glyphSize, lineWidth );
}


VECTOR2D STROKE_FONT::StringBoundaryLimits( const KIGFX::GAL* aGal, const UTF8& aText,
                                            const VECTOR2D& aGlyphSize,
                                            double          aGlyphThickness ) const
{
    VECTOR2D string_bbox;
    int      line_count = 1;
    double   maxX = 0.0, curX = 0.0;

    double curScale = 1.0;
    bool   in_overbar = false;
    bool   in_super_or_subscript = false;

    for( UTF8::uni_iter it = aText.ubegin(), end = aText.uend(); it < end; ++it )
    {
        if( *it == '\n' )
        {
            curX = 0.0;
            maxX = std::max( maxX, curX );
            ++line_count;
            continue;
        }

        // Handle tabs as locked to the nearest 4th column (counting in spaces)
        // The choice of spaces is somewhat arbitrary but sufficient for aligning text
        if( *it == '\t' )
        {
            double spaces = m_glyphBoundingBoxes->at( 0 ).GetEnd().x;
            double addlSpace = 3.0 * spaces - std::fmod( curX, 4.0 * spaces );

            // Add the remaining space (between 0 and 3 spaces)
            curX += addlSpace;

            // Tab ends an overbar
            in_overbar = false;
        }
        else if( *it == '~' )
        {
            if( ++it == end )
                break;

            if( *it == '~' )
            {
                // double ~ is really a ~ so go ahead and process the second one

                // so what's a triple ~?  It could be a real ~ followed by an overbar, or
                // it could be an overbar followed by a real ~.  The old algorithm did the
                // former so we will too....
            }
            else
            {
                // single ~ toggles overbar
                in_overbar = !in_overbar;
            }
        }
        else if( *it == '^' || *it == '_' )
        {
            auto lookahead = it;

            if( ++lookahead != end && *lookahead == '{' )
            {
                //  process superscript
                it = lookahead;
                in_super_or_subscript = true;
                curScale = 0.8;
                continue;
            }
        }
        else if( *it == '}' && in_super_or_subscript )
        {
            in_super_or_subscript = false;
            curScale = 1.0;
            continue;
        }
        // Overbar syntax is less precise so we have to have some special cases
        else if( in_overbar && ( *it == ' ' || *it == '}' || *it == ')' ) )
        {
            in_overbar = false;
        }

        // Index in the bounding boxes table
        int dd = (signed) *it - ' ';

        if( dd >= (int) m_glyphBoundingBoxes->size() || dd < 0 )
        {
            int substitute = *it == '\t' ? ' ' : '?';
            dd = substitute - ' ';
        }

        const BOX2D& box = m_glyphBoundingBoxes->at( dd );
        curX += box.GetEnd().x * curScale;
    }

    string_bbox.x = std::max( maxX, curX ) * aGlyphSize.x;
    string_bbox.x += aGlyphThickness;
    string_bbox.y = line_count * GetInterline( aGlyphSize.y );

    // For italic correction, take in account italic tilt
    if( aGal && aGal->IsFontItalic() )
        string_bbox.x += string_bbox.y * STROKE_FONT::ITALIC_TILT;

#if 0 //def OUTLINEFONT_DEBUG
    std::cerr << "STROKE_FONT::StringBoundaryLimits( " << ( aGal ? "aGal" : "nullptr" ) << ", \""
              << aText << "\", " << aGlyphSize << ", " << aGlyphThickness << " ) bbox "
              << string_bbox << " lines " << line_count << " interline "
              << GetInterline( aGlyphSize.y ) << std::endl;
#endif
    return string_bbox;
}


VECTOR2D STROKE_FONT::getBoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                                      TEXT_STYLE_FLAGS aTextStyle ) const
{
    // TODO: take glyph thickness into account!
    VECTOR2D bbox = StringBoundaryLimits( nullptr, aString, aGlyphSize, 0 );
#ifdef OUTLINEFONT_DEBUG
    std::cerr << "STROKE_FONT::getBoundingBox( " << aString << ", " << aGlyphSize << ", "
              << aTextStyle << " ) returns " << bbox << std::endl;
#endif
    return bbox;
}
