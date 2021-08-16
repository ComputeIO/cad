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
#if 1
    // TODO do we need to parse every time - have we already parsed?
    MARKUP::MARKUP_PARSER markupParser( aText );
    auto                  root = markupParser.Parse();

    GLYPH_LIST glyphs; // ignored
    //bool isMirrored = false; // TODO is this needed/does this information exist here?
    BOX2I boundingBox;
    (void) drawMarkup( &boundingBox, glyphs, root, VECTOR2D( 0, 0 ), aGlyphSize );
#ifdef DEBUG
    std::cerr << "STROKE_FONT::StringBoundaryLimits( ... ) returns " << boundingBox.GetSize() << std::endl;
#endif
    return boundingBox.GetSize();
#else
    // TODO: needs to be redone using MarkupParser

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
#endif
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

    int nth = 0;
    for( UTF8::uni_iter i = aText.ubegin(), end = aText.uend(); i < end; ++i )
    {
#ifdef DEBUG
        std::cerr << "Glyph #" << nth << " " << *i << " ";
#endif
        nth = nth + 1;

        // Index into bounding boxes table
        int dd = (signed) *i - ' ';

        if( dd >= (int) m_glyphBoundingBoxes->size() || dd < 0 )
        {
            int substitute = *i == '\t' ? ' ' : '?';
            dd = substitute - ' ';
        }

        if( aIsMirrored )
        {
            glyph = m_glyphs->at( dd )->Resize( glyphSize );
            double glyphWidth = glyph->BoundingBox().GetWidth();
            glyph = glyph->Translate( VECTOR2D( cursor.x - glyphWidth, cursor.y ) )->Mirror( aIsMirrored );
        }
        else
        {
            glyph = m_glyphs->at( dd )->Resize( glyphSize )->Translate( cursor );
        }

        if( dd == 0 )
        {
            // 'space' character - draw nothing, advance cursor position
            constexpr double spaceAdvance = 0.6;
            cursor.x += glyphSize.x * ( aIsMirrored ? -spaceAdvance : spaceAdvance );
        }
        else
        {
            constexpr double interGlyphSpaceMultiplier = 0.15;
            double           interGlyphSpace = glyphSize.x * interGlyphSpaceMultiplier;
            double           glyphAdvance = glyph->BoundingBox().GetWidth() + interGlyphSpace;
#ifdef DEBUG
            std::cerr << "glyph advance " << glyphAdvance
                      << " origin " << glyph->BoundingBox().GetOrigin()
                      << " width " << glyph->BoundingBox().GetWidth();
#endif
            if( aIsMirrored )
            {
                // cursor.x = glyphAdvance;
                cursor.x = glyph->BoundingBox().GetX() // - glyph->BoundingBox().GetWidth()
                           - 4 * interGlyphSpace;
            }
            else
            {
                cursor.x = glyph->BoundingBox().GetEnd().x + interGlyphSpace;
                // cursor.x += glyphAdvance;
            }
        }
#undef STROKE_FONT_DEBUG
#ifdef STROKE_FONT_DEBUG
#if 0
        // pass the bounding box rectangle instead of the actual glyph
        auto g = std::make_shared<STROKE_GLYPH>();
        g->AddPoint( glyph->BoundingBox().GetOrigin() );
        g->AddPoint( VECTOR2D( glyph->BoundingBox().GetEnd().x, glyph->BoundingBox().GetOrigin().y ) );
        g->AddPoint( VECTOR2D( glyph->BoundingBox().GetEnd().x, glyph->BoundingBox().GetEnd().y ) );
        g->AddPoint( VECTOR2D( glyph->BoundingBox().GetOrigin().x, glyph->BoundingBox().GetEnd().y ) );
        g->AddPoint( glyph->BoundingBox().GetOrigin() );
        g->Finalize();
        aGlyphs.push_back( g );
#else
        // add bounding box rectangle to glyph
        glyph->RaisePen();
        glyph->AddPoint( glyph->BoundingBox().GetOrigin() );
        glyph->AddPoint( VECTOR2D( glyph->BoundingBox().GetEnd().x, glyph->BoundingBox().GetOrigin().y ) );
        glyph->AddPoint( VECTOR2D( glyph->BoundingBox().GetEnd().x, glyph->BoundingBox().GetEnd().y ) );
        glyph->AddPoint( VECTOR2D( glyph->BoundingBox().GetOrigin().x, glyph->BoundingBox().GetEnd().y ) );
        glyph->AddPoint( glyph->BoundingBox().GetOrigin() );
        glyph->Finalize();
        aGlyphs.push_back( glyph );
#endif
#else
        aGlyphs.push_back( glyph );
#endif
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

#ifdef DEBUG
    std::cerr << "cursor[" << aText << "@" << aPosition << ":" << cursor.x << "," << cursor.y << "]";
#endif

    if( aBoundingBox )
    {
        aBoundingBox->SetOrigin( aPosition.x, aPosition.y );
        aBoundingBox->SetEnd( cursor.x, cursor.y + std::max( glyphSize.y, (double) barY ) );
#ifdef DEBUG
        std::cerr << "aBoundingBox[" << aBoundingBox->GetOrigin() << " " << aBoundingBox->GetSize() << "]";
#endif
        aBoundingBox->Normalize();
    }

    return VECTOR2I( cursor.x, aPosition.y );
}
