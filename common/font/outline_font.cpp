/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
 * Copyright (C) 2021 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Outline font class
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

#include <limits>
#include <wx/string.h>
#include <settings/settings_manager.h>
#include <harfbuzz/hb-ft.h>
#include <bezier_curves.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_poly_set.h>
#include <font/outline_font.h>
#include FT_GLYPH_H
#include FT_BBOX_H

FT_Library OUTLINE_FONT::mFreeType = nullptr;

OUTLINE_FONT::OUTLINE_FONT()
{
    if( !mFreeType )
    {
        //FT_Error ft_error = FT_Init_FreeType( &mFreeType );
        // TODO: handle ft_error
        FT_Init_FreeType( &mFreeType );
    }
}


bool OUTLINE_FONT::LoadFont( const wxString& aFontFileName )
{
    wxFileName fontFile( aFontFileName );
    wxString   fileName = fontFile.GetFullPath();
    // TODO: handle ft_error properly (now we just return false if load does not succeed)
    FT_Error ft_error = loadFace( fileName );

    if( ft_error )
    {
        // Try user dir
        fontFile.SetExt( "otf" );
        fontFile.SetPath( SETTINGS_MANAGER::GetUserSettingsPath() + wxT( "/fonts" ) );
        fileName = fontFile.GetFullPath();

        if( wxFile::Exists( fileName ) )
        {
            ft_error = loadFace( fileName );
        }
        else
        {
            fontFile.SetExt( "ttf" );
            fileName = fontFile.GetFullPath();
            if( wxFile::Exists( fileName ) )
            {
                ft_error = loadFace( fileName );
            }
        }
    }

    if( ft_error == FT_Err_Unknown_File_Format )
    {
        std::cerr << "The font file " << fileName << " could be opened and read, "
                  << "but it appears that its font format is unsupported." << std::endl;
    }
    else if( ft_error )
    {
        std::cerr << "ft_error " << ft_error << std::endl;
        return false;
    }
    else
    {
        m_fontName = aFontFileName;
        m_fontFileName = fileName;
    }

    return true;
}


FT_Error OUTLINE_FONT::loadFace( const wxString& aFontFileName )
{
    // TODO: check that going from wxString to char* with UTF-8
    // conversion for filename makes sense on any/all platforms
    return FT_New_Face( mFreeType, aFontFileName.mb_str( wxConvUTF8 ), 0, &mFace );
}


/**
 * Draw a string.
 *
 * @param aGal
 * @param aText is the text to be drawn.
 * @param aPosition is the text position in world coordinates.
 * @param aRotationAngle is the text rotation angle in radians.
 */
void OUTLINE_FONT::Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                         double aRotationAngle )
{
    if( aText.empty() )
        return;

    FT_Select_Charmap( mFace, FT_Encoding::FT_ENCODING_UNICODE );
    // FT_Set_Char_Size(
    //   <handle to face object>,
    //   <char_width in 1/64th of points>,
    //   <char_height in 1/64th of points>,
    //   <horizontal device resolution>,
    //   <vertical device resolution>
    // )
    // handle == variable (reference)
    // If only one of char_width/char_height == 0, it defaults to "same as the non-zero one"
    // Ditto if only one of horizontal/vertical resolution == 0
    // If both horizontal/vertical resolution == 0, 72 dpi is default
    //
    // 16 * 64 is chosen as an arbitrary default for width/height
    // TODO: come up with a non-arbitrary default (and/or use a better
    // value for H/V reso)
    FT_Set_Char_Size( mFace, 0, 16 * 64, 0, 0 );

    // Context needs to be saved before any transformations
    aGal->Save();
    aGal->Translate( aPosition );
    aGal->Rotate( -aRotationAngle );

    drawSingleLineText( aGal, aText );

    aGal->Restore();
}


/**
 * Compute the boundary limits of aText (the bounding box of all shapes).
 *
 * The overbar and alignment are not taken in account, '~' characters are skipped.
 *
 * @return a VECTOR2D giving the width and height of text.
 */
VECTOR2D OUTLINE_FONT::ComputeStringBoundaryLimits( const KIGFX::GAL* aGal, const UTF8& aText,
                                                    const VECTOR2D& aGlyphSize,
                                                    double          aGlyphThickness ) const
{
    assert( 1 == 0 ); // TODO!!!
}


/**
 * Compute the vertical position of an overbar, sometimes used in texts.
 *
 * This is the distance between the text base line and the overbar.
 *
 * @param aGlyphHeight is the height (vertical size) of the text.
 * @return the relative position of the overbar axis.
 */
double OUTLINE_FONT::ComputeOverbarVerticalPosition( double aGlyphHeight ) const
{
    assert( 1 == 0 ); // TODO!!!
}


/**
 * Compute the distance (interline) between 2 lines of text (for multiline texts).
 *
 * @param aGlyphHeight is the height (vertical size) of the text.
 * @return the interline.
 */
double OUTLINE_FONT::GetInterline( double aGlyphHeight ) const
{
    assert( 1 == 0 ); // TODO!!!
}


/**
 * Compute the X and Y size of a given text. The text is expected to be
 * a only one line text.
 *
 * @param aText is the text string (one line).
 * @return the text size.
 */
VECTOR2D OUTLINE_FONT::ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const
{
    assert( 1 == 0 ); // TODO!!!
}


static bool contourIsFilled( const CONTOUR& c )
{
    switch( c.orientation )
    {
    case FT_ORIENTATION_TRUETYPE: return c.winding == 1;

    case FT_ORIENTATION_POSTSCRIPT: return c.winding == -1;

    default: break;
    }

    return false;
}


static bool contourIsHole( const CONTOUR& c )
{
    return !contourIsFilled( c );
}


void OUTLINE_FONT::drawSingleLineText( KIGFX::GAL* aGal, const UTF8& aText )
{
    // Context needs to be saved before any transformations
    aGal->Save();

    std::vector<SHAPE_POLY_SET> glyphs;
    GetTextAsPolygon( glyphs, aText, aGal->GetGlyphSize(), aGal->IsTextMirrored() );

    aGal->SetIsFill( true );
    for( SHAPE_POLY_SET& glyph : glyphs )
    {
        aGal->DrawGlyph( glyph );
    }

    aGal->Restore();
}


void OUTLINE_FONT::GetTextAsPolygon( std::vector<SHAPE_POLY_SET>& aGlyphs, const UTF8& aText,
                                     const VECTOR2D& aGlyphSize, bool aIsMirrored ) const
{
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, aText.c_str(), -1, 0, -1 );

    // guess direction, script, and language based on contents
    hb_buffer_guess_segment_properties( buf );

    unsigned int         glyphCount;
    hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( buf, &glyphCount );
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( buf, &glyphCount );
    hb_font_t*           referencedFont = hb_ft_font_create_referenced( mFace );

    hb_ft_font_set_funcs( referencedFont );
    hb_shape( referencedFont, buf, NULL, 0 );

    const double mirror_factor = ( aIsMirrored ? 1 : -1 );
    // TODO: xyscaler is determined with the Stetson method to make
    // debugging easier; root cause for why scaling does not seem to
    // work is unknown (but most likely trivial)
    const double xyscaler = 1.0e3;
    const double x_scale_factor = mirror_factor * aGlyphSize.x / xyscaler;
    const double y_scale_factor = aGlyphSize.y / xyscaler;
    const double advance_scale_factor = 1;
    const double advance_x_factor = advance_scale_factor;
    const double advance_y_factor = advance_scale_factor;

#ifdef DEBUG //STROKEFONT
    std::cerr << "GetTextAsPolygon " << aText << " glyphSize " << aGlyphSize << std::endl;
#endif

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;

    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        hb_glyph_position_t& pos = glyphPos[i];
        int                  codepoint = glyphInfo[i].codepoint;

        FT_Load_Glyph( mFace, codepoint, FT_LOAD_NO_BITMAP );

        FT_GlyphSlot       glyph = mFace->glyph;
        const unsigned int bufsize = 512;
        char               glyphName[bufsize];
        FT_Get_Glyph_Name( mFace, glyph->glyph_index, &glyphName[0], bufsize );

        // contours is a collection of all outlines in the glyph;
        // example: glyph for 'o' generally contains 2 contours,
        // one for the glyph outline and one for the hole
        CONTOURS contours;

        OUTLINE_DECOMPOSER decomposer( glyph->outline );
        decomposer.OutlineToSegments( &contours );

        SHAPE_POLY_SET             poly;
        std::vector<SHAPE_SIMPLE*> holes;
        std::vector<SHAPE_SIMPLE*> outlines;
        std::vector<VECTOR2D>      ptListScaled;

        for( CONTOUR c : contours )
        {
            POINTS points = c.points;
            int    ptCount = 0;
            ptListScaled.clear();

            for( const VECTOR2D& v : points )
            {
                VECTOR2D pt( -( v.x + cursor_x ), -( v.y + cursor_y ) );
                VECTOR2D scaledPt( pt.x * x_scale_factor, pt.y * y_scale_factor );
                ptListScaled.push_back( scaledPt );
                ptCount++;
            }

            SHAPE_SIMPLE* shape = new SHAPE_SIMPLE();
            for( const VECTOR2D& p : ptListScaled )
            {
                shape->Append( p.x, p.y );
            }

            if( contourIsHole( c ) )
            {
                holes.push_back( shape );
            }
            else
            {
                outlines.push_back( shape );
            }
        }

        if( outlines.size() < 1 )
        {
            // std::cerr << "Error: no outlines for [" << glyphName << "]!" << std::endl;
        }

        for( SHAPE_SIMPLE* outline : outlines )
        {
            if( outline->PointCount() )
            {
                poly.AddOutline( outline->Vertices() );
            }
        }

        int nthHole = 0;
        for( SHAPE_SIMPLE* hole : holes )
        {
            if( hole->PointCount() )
            {
                VECTOR2I firstPoint = hole->GetPoint( 0 );
                //SHAPE_SIMPLE *outlineForHole = nullptr;
                int nthOutline = -1;
                int n = 0;
                for( SHAPE_SIMPLE* outline : outlines )
                {
                    if( outline->PointInside( firstPoint ) )
                    {
                        //outlineForHole = outline;
                        nthOutline = n;
                        break;
                    }
                    n++;
                }
                if( nthOutline > -1 )
                {
                    poly.AddHole( hole->Vertices(), n );
                }
            }
            delete hole;
            nthHole++;
        }

        aGlyphs.push_back( poly );

        for( SHAPE_SIMPLE* outline : outlines )
            delete outline;

        cursor_x += ( pos.x_advance * advance_x_factor );
        cursor_y += ( pos.y_advance * advance_y_factor );
    }

    hb_buffer_destroy( buf );
}
