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
#include <gr_text.h>
#include <settings/settings_manager.h>
#include <newstroke_font.h>
#include <font/stroke_font.h>
#include <parser/markup_parser.h>
#include <cmath>

using namespace KIFONT;

GLYPH_LIST*         g_defaultFontGlyphs = nullptr;   ///< Glyph list
std::vector<BOX2D>* g_defaultFontGlyphBoundingBoxes; ///< Bounding boxes of the glyphs


STROKE_FONT::STROKE_FONT() : m_glyphs( nullptr ), m_glyphBoundingBoxes( nullptr )
{
}


bool STROKE_FONT::LoadFont( const wxString& aFontName )
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


double STROKE_FONT::GetInterline( double aGlyphHeight ) const
{
    // Do not add the glyph thickness to the interline.  This makes bold text line-spacing
    // different from normal text, which is poor typography.
    double ret = aGlyphHeight * INTERLINE_PITCH_RATIO;
#ifdef FOOBAR //DEBUG
    std::cerr << "STROKE_FONT::GetInterline( " << aGlyphHeight << " ) const --> " << ret
              << std::endl;
#endif
    return ret;
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


VECTOR2D STROKE_FONT::Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                            const VECTOR2D& aOrigin, const EDA_ANGLE& aRotationAngle ) const
{
    if( aText.empty() )
        return VECTOR2D( 0.0, 0.0 );

#ifdef DEBUG
    std::cerr << "STROKE_FONT::Draw( " << ( aGal ? "[aGal]" : "nullptr" ) << ", \"" << aText
              << "\", " << aPosition << ", " << aRotationAngle << " )"
              << " aGal line width " << (aGal ? aGal->GetLineWidth() : 0.0f)
              << std::endl;
    // debug circle width
    double dbg = 100000;
#endif

    if( aGal )
    {
        // Context needs to be saved before any transformations
        aGal->Save();

#ifdef DEBUG
        double lw = aGal->GetLineWidth();
        aGal->SetLineWidth( 10.0 );
        //aGal->SetIsStroke( true );
        COLOR4D oldColor = aGal->GetStrokeColor();
        aGal->SetStrokeColor( COLOR4D( 0, 0, 0.6, 0.5 ) );
        aGal->DrawCircle( aOrigin, dbg );
#endif
        aGal->Translate( aOrigin );
#ifdef DEBUG
        aGal->SetStrokeColor( COLOR4D( 0.7, 0.8, 0.6, 0.5 ) );
        aGal->DrawCircle( VECTOR2D( 0, 0 ), 1.35 * dbg );
#endif
        aGal->Rotate( aRotationAngle.Invert().AsRadians() );
#ifdef DEBUG
        aGal->SetStrokeColor( COLOR4D( 0.2, 0.3, 0.4, 0.5 ) );
        aGal->DrawCircle( VECTOR2D( 0, 0 ), 1.3 * dbg );

        aGal->SetStrokeColor( COLOR4D( 0.4, 0.5, 0.6, 0.5 ) );

        aGal->DrawSegment( VECTOR2D( 0, 0 ), VECTOR2D( 1.3 * dbg, 0 ), 10.0 );
#endif
        aGal->Translate( aPosition - aOrigin );
#ifdef DEBUG
        aGal->SetStrokeColor( COLOR4D( 0.8, 0.6, 0.4, 0.5 ) );
        aGal->DrawCircle( VECTOR2D( 0, 0 ), 1.2 * dbg );

        aGal->SetStrokeColor( COLOR4D( 0.4, 0.3, 0.2, 0.5 ) );
        aGal->DrawSegment( VECTOR2D( 0, 0 ), VECTOR2D( 1.2 * dbg, 0 ), 10.0 );

        aGal->SetStrokeColor( oldColor );
        aGal->SetLineWidth( lw );
#endif
    }

    // Single line height
    // TODO: default glyphSize for !aGal is just a guess!
    //int            lineHeight = KiROUND( GetInterline( glyphSize.y ) );

    if( aGal )
    {
#if 0
        const VECTOR2D glyphSize = aGal ? VECTOR2D( aGal->GetGlyphSize() ) : VECTOR2D( 16.0, 16.0 );
        switch( aGal->GetVerticalJustify() )
        {
        case GR_TEXT_VJUSTIFY_TOP: aGal->Translate( VECTOR2D( 0, glyphSize.y ) ); break;

        case GR_TEXT_VJUSTIFY_CENTER: aGal->Translate( VECTOR2D( 0, glyphSize.y / 2.0 ) ); break;

        case GR_TEXT_VJUSTIFY_BOTTOM: break;

        default: break;
        }
#endif

        aGal->SetIsStroke( true );
        //aGal->SetIsFill( false );

        if( aGal->IsFontBold() ) {
#ifdef DEBUG
            std::cerr << "[line width was " << aGal->GetLineWidth() << ", ";
#endif
            aGal->SetLineWidth( aGal->GetLineWidth() * BOLD_FACTOR );
#ifdef DEBUG
            std::cerr << "is " << aGal->GetLineWidth() << "]\n";
#endif
        }
    }

#if 0
    VECTOR2D boundingBox( 0.0, 0.0 );
    // Split multiline strings into separate ones and draw them line by line
    size_t begin = 0;
    size_t newlinePos = aText.find( '\n' );

    VECTOR2D lineBoundingBox;
    while( newlinePos != aText.npos )
    {
        size_t length = newlinePos - begin;

        lineBoundingBox = drawSingleLineText( aGal, aText.substr( begin, length ) );
        boundingBox.x = fmax( boundingBox.x, lineBoundingBox.x );
        boundingBox.y += lineBoundingBox.y;

        if( aGal )
            aGal->Translate( VECTOR2D( 0.0, lineHeight ) );

        begin = newlinePos + 1;
        newlinePos = aText.find( '\n', begin );
    }

    // Draw the last (or the only one) line
    if( !aText.empty() )
    {
        drawSingleLineText( aGal, aText.substr( begin ) );
        boundingBox.x = fmax( boundingBox.x, lineBoundingBox.x );
        boundingBox.y += lineBoundingBox.y;
    }
#else
#ifdef DEBUG
    std::cerr << "[aGal line width " << (aGal ? aGal->GetLineWidth() : 0.0f) << "]\n";
#endif
    VECTOR2D boundingBox = drawSingleLineText( aGal, aText );
#endif

    if( aGal )
        aGal->Restore();

    return boundingBox;
}


VECTOR2D STROKE_FONT::drawSingleLineText( KIGFX::GAL* aGal, const UTF8& aText,
                                          const VECTOR2D& aPosition, const EDA_ANGLE& aAngle ) const
{
#ifdef DEBUG
    std::cerr << "drawSingleLineText(...," << aText << ",...) aGal line width "
              << (aGal ? aGal->GetLineWidth() : 0.0f) << std::endl;
#endif
    // TODO default for baseGlyphSize just a guess
    VECTOR2D baseGlyphSize( aGal ? aGal->GetGlyphSize() : VECTOR2D( 16.0, 16.0 ) );
    bool     underlined = aGal && aGal->IsFontUnderlined();
    bool     italic = aGal && aGal->IsFontItalic();
    bool     mirrored = aGal && aGal->IsTextMirrored();
    double   italic_tilt = italic ? ( mirrored ? -ITALIC_TILT : ITALIC_TILT ) : 0;
    double   overbar_italic_comp = computeOverbarVerticalPosition( aGal ) * italic_tilt;
    VECTOR2D textSize = ComputeTextLineSize( aGal, aText );
    double   half_thickness = aGal ? aGal->GetLineWidth() / 2 : 1;
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
    std::cerr << "STROKE_FONT::drawSingleLineText( " << ( aGal ? "[aGal]" : "nullptr" ) << ", \""
              << aText << "\", " << aPosition << ", " << aAngle << " )\n"
              << "baseGlyphSize " << baseGlyphSize << " textSize " << textSize << " half_thickness "
              << half_thickness << " overbar_italic_comp " << overbar_italic_comp
              << ( italic ? " " : " !" ) << "italic" << ( underlined ? " " : " !" ) << "underlined"
              << ( mirrored ? " " : " !" ) << "mirrored"
              << " xOffset " << xOffset
              << " aGal line width " << (aGal ? aGal->GetLineWidth() : 0.0f);
    //if( aGal )
    //    std::cerr << " aGal " << *aGal;
    std::cerr << std::endl;
    int dbg = 20000;
#endif

#ifdef DEBUG
    if( aGal )
    {
        COLOR4D oldColor = aGal->GetStrokeColor();
        double lw = aGal->GetLineWidth();
        aGal->SetLineWidth( 10.0 );
        if( mirrored )
            aGal->SetStrokeColor( COLOR4D( 1, 1, 0, 0.08 ) );
        else
            aGal->SetStrokeColor( COLOR4D( 1, 0, 1, 0.08 ) );
        aGal->DrawCircle( VECTOR2D( xOffset, yOffset ), 1.1 * dbg );
        aGal->DrawRectangle( VECTOR2D( -dbg / 2, -dbg / 2 ), VECTOR2D( dbg, dbg ) );
        aGal->SetLineWidth( lw );
        aGal->SetStrokeColor( oldColor );
    }
#endif

    if( aGal )
    {
        // Context needs to be saved before any transformations
        aGal->Save();
        // First adjust: the text X position is corrected by half_thickness
        // because when the text with thickness is draw, its full size is textSize,
        // but the position of lines is half_thickness to textSize - half_thickness
        // so we must translate the coordinates by half_thickness on the X axis
        // to place the text inside the 0 to textSize X area.
        aGal->Translate( VECTOR2D( half_thickness, 0 ) );
#if 0
        // TODO mirrored!

        // Adjust the text position to the given horizontal justification
        switch( aGal->GetHorizontalJustify() )
        {
            //case GR_TEXT_HJUSTIFY_CENTER:
            //  aGal->Translate( VECTOR2D( -textSize.x / 2.0, 0 ) ); break;

        case GR_TEXT_HJUSTIFY_RIGHT:
            aGal->Translate( VECTOR2D( -textSize.x / 2, 0 ) );

            if( !mirrored )
                aGal->Translate( VECTOR2D( -textSize.x, 0 ) );
            break;

        case GR_TEXT_HJUSTIFY_LEFT:
            aGal->Translate( VECTOR2D( textSize.x / 2, 0 ) );

            if( mirrored )
                aGal->Translate( VECTOR2D( -textSize.x, 0 ) );
            break;

        default: break;
        }
#endif

        // center horizontally
        VECTOR2D hv( -textSize.x / 2, textSize.y / 4 );
#ifdef DEBUG
        std::cerr << "Translating with " << hv << "!" << std::endl;
        double lw = aGal->GetLineWidth();
        aGal->SetLineWidth( 10.0 );
        aGal->DrawRectangle( VECTOR2D(0,0), hv);
        aGal->DrawRectangle( VECTOR2D(-textSize.x / 2, -textSize.y / 2), textSize );
#endif
        aGal->Translate( hv );
#ifdef DEBUG
        COLOR4D oldColor = aGal->GetStrokeColor();
        aGal->SetStrokeColor( COLOR4D( 0, 1, 0, 0.2 ) );
        aGal->DrawRectangle( VECTOR2D(-textSize.x / 2, -textSize.y / 2), textSize );
        aGal->SetStrokeColor( oldColor );
        aGal->SetLineWidth( lw );
#endif
    }

    MARKUP::MARKUP_PARSER markup_parser( aText );
    markup_parser.Parse();

    // The overbar is indented inward at the beginning of an italicized section, but
    // must not be indented on subsequent letters to ensure that the bar segments
    // overlap.
    bool     last_had_overbar = false;
    bool     in_overbar = false;
    bool     in_super_or_subscript = false;
    VECTOR2D glyphSize = baseGlyphSize;

    // Allocate only once (for performance)
    std::vector<VECTOR2D> ptListScaled;
    int                   char_count = 0;

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
                if( aGal && aGal->IsFontItalic() )
                    overbar_start_x += overbar_italic_comp;

                last_had_overbar = true;
            }

            VECTOR2D startOverbar( overbar_start_x, overbar_start_y );
            VECTOR2D endOverbar( overbar_end_x, overbar_end_y );

            if( aGal )
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

            if( aGal )
                aGal->DrawLine( startUnderline, endUnderline );
        }

        for( const std::vector<VECTOR2D>* ptList : *glyph )
        {
            int ptCount = 0;
            ptListScaled.clear();

#ifdef DEBUG
            if( aGal )
            {
                COLOR4D oldColor = aGal->GetStrokeColor();
                if( mirrored )
                    aGal->SetStrokeColor( COLOR4D( 0, 1, 1, 0.05 ) );
                else
                    aGal->SetStrokeColor( COLOR4D( 1, 0, 0, 0.05 ) );
                aGal->DrawCircle( VECTOR2D( xOffset, yOffset ), 20000 );
                aGal->SetStrokeColor( oldColor );
            }
#endif
            for( const VECTOR2D& pt : *ptList )
            {
                VECTOR2D scaledPt( pt.x * glyphSize.x + xOffset, pt.y * glyphSize.y + yOffset );

                scaledPt.x += -scaledPt.y * italic_tilt;

                ptListScaled.push_back( scaledPt );
                ptCount++;
            }

            if( aGal ) {
                aGal->DrawPolyline( &ptListScaled[0], ptCount );
            }
        }

        char_count++;
        xOffset += glyphSize.x * bbox.GetEnd().x;
    }

    if( aGal )
        aGal->Restore();

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

#if 0 //def DEBUG
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
#ifdef DEBUG
    std::cerr << "STROKE_FONT::getBoundingBox( " << aString << ", " << aGlyphSize << ", "
              << aTextStyle << " ) returns " << bbox << std::endl;
#endif
    return bbox;
}
