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
#include <wx/filename.h>
#include <wx/textfile.h>
#include <gr_text.h>
#include <settings/settings_manager.h>
#include <newstroke_font.h>
#include <font/glyph.h>
#include <font/stroke_font.h>
#include <parser/markup_parser.h>
#include <cmath>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <trigo.h>

using namespace KIFONT;

bool                g_defaultFontInitialized = false;
GLYPH_LIST          g_defaultFontGlyphs;             ///< Glyph list
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
    if( !g_defaultFontInitialized )
    {
        g_defaultFontGlyphs.reserve( aNewStrokeFontSize );

        g_defaultFontGlyphBoundingBoxes = new std::vector<BOX2D>;
        g_defaultFontGlyphBoundingBoxes->reserve( aNewStrokeFontSize );

        for( int j = 0; j < aNewStrokeFontSize; j++ )
        {
            auto   glyph = std::make_shared<STROKE_GLYPH>();
            double glyphStartX = 0.0;
            //double                        glyphEndX = 0.0;
            //double                        glyphWidth = 0.0;

            std::vector<VECTOR2D>* pointList = nullptr;

            int strokes = 0;
            int i = 0;

            while( aNewStrokeFont[j][i] )
            {
                if( aNewStrokeFont[j][i] == ' ' && aNewStrokeFont[j][i + 1] == 'R' )
                    strokes++;

                i += 2;
            }

            //glyph->reserve( strokes + 1 );

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
                    //glyphEndX = ( coordinate[1] - 'R' ) * STROKE_FONT_SCALE;
                    //glyphWidth = glyphEndX - glyphStartX;
                }
                else if( ( coordinate[0] == ' ' ) && ( coordinate[1] == 'R' ) )
                {
                    //if( pointList )
                    //pointList->shrink_to_fit();

                    // Raise pen
                    //pointList = nullptr;
                    glyph->RaisePen();
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
                        //glyph->push_back( pointList );
                    }

                    glyph->AddPoint( point );
                    //pointList->push_back( point );
                }

                i += 2;
            }

            glyph->Finalize();

            // Compute the bounding box of the glyph
            g_defaultFontGlyphBoundingBoxes->emplace_back( glyph->BoundingBox() );
            g_defaultFontGlyphs.push_back( glyph );
            m_maxGlyphWidth =
                    std::max( m_maxGlyphWidth, g_defaultFontGlyphBoundingBoxes->back().GetWidth() );
        }

        g_defaultFontInitialized = true;
    }

    m_glyphs = &g_defaultFontGlyphs;
    m_glyphBoundingBoxes = g_defaultFontGlyphBoundingBoxes;
    return true;
}


std::shared_ptr<GLYPH> STROKE_FONT::processGlyph( std::string aGlyphString )
{
    // handle glyph data
    std::shared_ptr<STROKE_GLYPH> glyph;
    double                        glyphStartX = 0.0;
    //double       glyphEndX = 0.0;
    int strokes = 0;
    int i;

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

    //glyph->reserve( strokes + 1 );

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
            //glyphEndX = ( coordinate[1] - 'R' ) * STROKE_FONT_SCALE;
        }
        else if( ( coordinate[0] == ' ' ) && ( coordinate[1] == 'R' ) )
        {
            glyph->RaisePen();
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

            //if( !pointList )
            //{
            //pointList = new std::vector<VECTOR2D>;
            //glyph->push_back( pointList );
            //}

            glyph->AddPoint( point );
        }

        i += 2;
    }

    glyph->Finalize();

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

        if( !continuationLine && !glyphString.empty() )
        {
            // Process current glyphString contents first
            std::shared_ptr<GLYPH> glyph = processGlyph( glyphString );

            // Compute the bounding box of the glyph
            fontGlyphBoundingBoxes->emplace_back( glyph->BoundingBox() );
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
            std::shared_ptr<GLYPH> glyph = processGlyph( glyphString );

            // Compute the bounding box of the glyph
            fontGlyphBoundingBoxes->emplace_back( glyph->BoundingBox() );
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


double STROKE_FONT::ComputeOverbarVerticalPosition( double aGlyphHeight ) const
{
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
    // TODO do we need to parse every time - have we already parsed?
    MARKUP::MARKUP_PARSER markupParser( aText );
    auto                  root = markupParser.Parse();

    GLYPH_LIST glyphs; // ignored
    BOX2I      boundingBox;

    (void) drawMarkup( &boundingBox, glyphs, root, VECTOR2D( 0, 0 ), aGlyphSize );

    return boundingBox.GetSize();
}


VECTOR2D STROKE_FONT::getBoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                                      TEXT_STYLE_FLAGS aTextStyle ) const
{
    // TODO: take glyph thickness into account!
    return StringBoundaryLimits( nullptr, aString, aGlyphSize, 0 );
}


VECTOR2I STROKE_FONT::GetTextAsPolygon( BOX2I* aBoundingBox, GLYPH_LIST& aGlyphs, const UTF8& aText,
                                        const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                                        const EDA_ANGLE& aAngle, bool aIsMirrored,
                                        TEXT_STYLE_FLAGS aTextStyle ) const
{
    std::shared_ptr<GLYPH> glyph = nullptr;
    wxPoint cursor( aPosition );
    VECTOR2D glyphSize( aGlyphSize );

    if( aTextStyle & TEXT_STYLE::SUBSCRIPT || aTextStyle & TEXT_STYLE::SUPERSCRIPT )
    {
        constexpr double subscriptSuperscriptMultiplier = 0.7;
        glyphSize.x *= subscriptSuperscriptMultiplier;
        glyphSize.y *= subscriptSuperscriptMultiplier;

        if( aTextStyle & TEXT_STYLE::SUBSCRIPT )
        {
            constexpr double subscriptVerticalMultiplier = 0.3;
            cursor.y += glyphSize.y * subscriptVerticalMultiplier;
        }
        else
        {
            constexpr double superscriptVerticalMultiplier = 0.5;
            cursor.y -= glyphSize.y * superscriptVerticalMultiplier;
        }
    }

    for( UTF8::uni_iter i = aText.ubegin(), end = aText.uend(); i < end; ++i )
    {
        // Index into bounding boxes table
        int dd = (signed) *i - ' ';

        if( dd >= (int) m_glyphBoundingBoxes->size() || dd < 0 )
        {
            switch( *i )
            {
                case '\t':
                    // TAB->SPACE
                    dd = 0;
                    break;
                default:
                    // everything else is turned into a '?'
                    dd = '?' - ' ';
            }
        }

        glyph = m_glyphs->at( dd )->Resize( glyphSize )->Translate( cursor );

        if( dd == 0 )
        {
            // 'space' character - draw nothing, advance cursor position
            constexpr double spaceAdvance = 0.6;
            cursor.x += glyphSize.x * spaceAdvance;
        }
        else
        {
            constexpr double interGlyphSpaceMultiplier = 0.15;
            double           interGlyphSpace = glyphSize.x * interGlyphSpaceMultiplier;
            cursor.x = glyph->BoundingBox().GetEnd().x + interGlyphSpace;
            aGlyphs.push_back( glyph );
        }
    }

    int barY = 0;
    if( aTextStyle & TEXT_STYLE::OVERBAR )
    {
        auto overbarGlyph = std::make_shared<STROKE_GLYPH>();

        int left = aPosition.x;
        int right = cursor.x;
        barY = cursor.y - ComputeOverbarVerticalPosition( glyphSize.y );

        overbarGlyph->AddPoint( VECTOR2D( left, barY ) );
        overbarGlyph->AddPoint( VECTOR2D( right, barY ) );
        overbarGlyph->Finalize();

        aGlyphs.push_back( overbarGlyph );
    }

    // TODO: bounding box computation when mirrored - check?
    if( aBoundingBox )
    {
        aBoundingBox->SetOrigin( aPosition.x, aPosition.y );
        aBoundingBox->SetEnd( cursor.x, cursor.y + std::max( glyphSize.y, (double) barY ) );
        aBoundingBox->Normalize();
    }

    return VECTOR2I( cursor.x, aPosition.y );
}
