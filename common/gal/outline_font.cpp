/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
 * Copyright (C) 2016-2021 Kicad Developers, see change_log.txt for contributors.
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

#include <wx/string.h>
#include <settings/settings_manager.h>
#include <harfbuzz/hb-ft.h>
#include <gal/outline_font.h>
#include FT_GLYPH_H
#include FT_BBOX_H
#include FT_OUTLINE_H
#include <bezier_curves.h>

using namespace KIGFX;

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
#ifdef DEBUG
    std::cerr << "OUTLINE_FONT::LoadFont( \"" << aFontFileName << "\" )" << std::endl;
#endif

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
        m_fontFileName = fileName;
    }

#ifdef DEBUG
    std::cerr << aFontFileName << " loaded from " << fileName << std::endl;
    std::cerr << "Family " << mFace->family_name << " Style " << mFace->style_name << " has "
              << mFace->num_faces << " faces and " << mFace->num_glyphs << " glyphs." << std::endl;
#endif
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
void OUTLINE_FONT::Draw( GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                         double aRotationAngle )
{
    if( aText.empty() )
        return;

#ifdef DEBUG
    std::cerr << "OUTLINE_FONT::Draw(" << aText << ")" << std::endl;
#endif

    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, aText.c_str(), -1, 0, -1 );

    // guess direction, script, and language based on contents
    hb_buffer_guess_segment_properties( buf );

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

    hb_font_t* font = hb_ft_font_create_referenced( mFace );
    hb_ft_font_set_funcs( font );
    hb_shape( font, buf, NULL, 0 );

    unsigned int         glyph_count;
    hb_glyph_info_t*     glyph_info = hb_buffer_get_glyph_infos( buf, &glyph_count );
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions( buf, &glyph_count );
    hb_position_t        cursor_x = 0;
    hb_position_t        cursor_y = 0;
#ifdef DEBUG
#ifdef FOOFAA
    for( unsigned int i = 0; i < glyph_count; i++ )
    {
        hb_codepoint_t glyphid = glyph_info[i].codepoint;
        hb_position_t  x_offset = glyph_pos[i].x_offset;
        hb_position_t  y_offset = glyph_pos[i].y_offset;
        hb_position_t  x_advance = glyph_pos[i].x_advance;
        hb_position_t  y_advance = glyph_pos[i].y_advance;
        /* draw_glyph(glyphid, cursor_x + x_offset, cursor_y + y_offset); */
        std::cerr << "glyph #" << i << "/" << glyph_count << " id " << glyphid << " x " << cursor_x
                  << "+" << x_advance << " offset " << x_offset << " y " << cursor_y << "+"
                  << y_advance << " offset " << y_offset << std::endl;
        cursor_x += x_advance;
        cursor_y += y_advance;
    }
#endif
#endif

    // Context needs to be saved before any transformations
    aGal->Save();

    aGal->Translate( aPosition );
    aGal->Rotate( -aRotationAngle );
    const VECTOR2D& glyphSize = aGal->GetGlyphSize();

#ifdef DEBUG
    std::cerr << "GAL glyphSize == " << glyphSize << std::endl;
#endif

#ifdef FOOBAR
    // Single line height
    int lineHeight = KiROUND( GetInterline( m_gal->GetGlyphSize().y ) );
    int lineCount = linesCount( aText );
    //const VECTOR2D& glyphSize = m_gal->GetGlyphSize();

    // align the 1st line of text
    switch( m_gal->GetVerticalJustify() )
    {
    case GR_TEXT_VJUSTIFY_TOP: m_gal->Translate( VECTOR2D( 0, glyphSize.y ) ); break;

    case GR_TEXT_VJUSTIFY_CENTER: m_gal->Translate( VECTOR2D( 0, glyphSize.y / 2.0 ) ); break;

    case GR_TEXT_VJUSTIFY_BOTTOM: break;

    default: break;
    }

    if( lineCount > 1 )
    {
        switch( m_gal->GetVerticalJustify() )
        {
        case GR_TEXT_VJUSTIFY_TOP: break;

        case GR_TEXT_VJUSTIFY_CENTER:
            m_gal->Translate( VECTOR2D( 0, -( lineCount - 1 ) * lineHeight / 2 ) );
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            m_gal->Translate( VECTOR2D( 0, -( lineCount - 1 ) * lineHeight ) );
            break;
        }
    }

    m_gal->SetIsStroke( false );
    m_gal->SetIsFill( true );

    if( m_gal->IsFontBold() )
    {
        // TODO: figure out how to do real bold - preferably by using
        // bold font instead of normal (if this is normal, and there
        // is a bold version)
        m_gal->SetLineWidth( m_gal->GetLineWidth() * 1.3 );
    }

    // Split multiline strings into separate ones and draw them line by line
    size_t begin = 0;
    size_t newlinePos = aText.find( '\n' );

    while( newlinePos != aText.npos )
    {
        size_t length = newlinePos - begin;

        drawSingleLineText( aText.substr( begin, length ) );
        m_gal->Translate( VECTOR2D( 0.0, lineHeight ) );

        begin = newlinePos + 1;
        newlinePos = aText.find( '\n', begin );
    }

    // Draw the last (or the only one) line
    if( !aText.empty() )
        drawSingleLineText( aText.substr( begin ) );

#endif // FOOBAR

    drawSingleLineText( aGal, buf, font );
    hb_buffer_destroy( buf );
    aGal->Restore();
}


/**
 * Compute the boundary limits of aText (the bounding box of all shapes).
 *
 * The overbar and alignment are not taken in account, '~' characters are skipped.
 *
 * @return a VECTOR2D giving the width and height of text.
 */
VECTOR2D OUTLINE_FONT::ComputeStringBoundaryLimits( const GAL* aGal, const UTF8& aText,
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
VECTOR2D OUTLINE_FONT::ComputeTextLineSize( const GAL* aGal, const UTF8& aText ) const
{
    assert( 1 == 0 ); // TODO!!!
}


void OUTLINE_FONT::drawSingleLineText( GAL* aGal, hb_buffer_t* aText, hb_font_t* aFont )
{
    const VECTOR2D& galGlyphSize = aGal->GetGlyphSize();

    // Context needs to be saved before any transformations
    aGal->Save();

    unsigned int         glyphCount;
    hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( aText, &glyphCount );
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( aText, &glyphCount );

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;

#ifdef DEBUG
#ifdef FOOFAA
    const int GLYPH_NAME_LEN = 256;
    char      glyph_name[GLYPH_NAME_LEN];
    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        hb_codepoint_t glyphid = glyphInfo[i].codepoint;
        hb_position_t  x_offset = glyphPos[i].x_offset;
        hb_position_t  y_offset = glyphPos[i].y_offset;
        hb_position_t  x_advance = glyphPos[i].x_advance;
        hb_position_t  y_advance = glyphPos[i].y_advance;
        /* draw_glyph(glyphid, cursor_x + x_offset, cursor_y + y_offset); */
        std::cerr << "Glyph #" << i << "/" << glyphCount << " id " << glyphid << " x " << cursor_x
                  << "+" << x_advance << " offset " << x_offset << " y " << cursor_y << "+"
                  << y_advance << " offset " << y_offset << std::endl;
        cursor_x += x_advance;
        cursor_y += y_advance;
    }
#endif
#endif

    const double mirror_factor = ( aGal->IsTextMirrored() ? 1 : -1 );
    // TODO: xyscaler is determined with the Stetson method to make
    // debugging easier; root cause for why scaling does not seem to
    // work is unknown (but most likely trivial)
    const double xyscaler = 2.0e3;
    const double x_scale_factor = mirror_factor * galGlyphSize.x / xyscaler;
    const double y_scale_factor = galGlyphSize.y / xyscaler;
    // TODO: why 2x? not sure why this is needed, but without it
    // advance (at least X advance, haven't yet witnessed non-zero Y
    // advance) seems to be about 0.5x what is required
    const double advance_scale_factor = 2;
    const double advance_x_factor = advance_scale_factor;
    const double advance_y_factor = advance_scale_factor;
#ifdef DEBUG
#ifdef FOOFAA
    std::cerr << "mirror_factor " << mirror_factor << " xyscaler " << xyscaler << " x_scale_factor "
              << x_scale_factor << " y_scale_factor " << y_scale_factor << " advance_scale_factor "
              << advance_scale_factor << " advance_x_factor " << advance_x_factor
              << " advance_y_factor " << advance_y_factor << std::endl;
#endif
#endif

    cursor_x = 0;
    cursor_y = 0;
    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        int                  cluster = glyphInfo[i].cluster;
        int                  codepoint = glyphInfo[i].codepoint;
        hb_glyph_position_t& pos = glyphPos[i];
#ifdef DEBUG
#ifdef FOOFAA
        std::cerr << "ch[" << i << "] cluster " << cluster << " codepoint " << codepoint
                  << " advance " << pos.x_advance << "," << pos.y_advance << " offset "
                  << pos.x_offset << "," << pos.y_offset << std::endl;
#endif
#endif

        FT_Load_Glyph( mFace, codepoint, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE );

        FT_GlyphSlot glyph = mFace->glyph;
        unsigned int glyph_index = glyph->glyph_index;

#ifdef DEBUG
#ifdef FOOFAA
        FT_Get_Glyph_Name( mFace, codepoint, glyph_name, GLYPH_NAME_LEN );
        std::cerr << "glyph name [" << glyph_name << "] " << glyph_index << std::endl;
#endif
#endif
#ifdef CONTOUR_CACHE
        if( !mContourCache.count( glyph_index ) )
        {
            mContourCache[glyph_index] = outlineToStraightSegments( glyph->outline );
        }
        POINTS_LIST contours = mContourCache[glyph_index];
#else
        POINTS_LIST contours = outlineToStraightSegments( glyph->outline );
#endif

        std::vector<VECTOR2D> ptListScaled;
        for( POINTS points : contours )
        {
            int ptCount = 0;
            ptListScaled.clear();
            for( const VECTOR2D& v : points )
            {
                VECTOR2D pt( -( v.x + cursor_x ), -( v.y + cursor_y ) );
                VECTOR2D scaledPt( pt.x * x_scale_factor, pt.y * y_scale_factor );
                ptListScaled.push_back( scaledPt );
                ptCount++;
            }

            // always fill outline
            //
            // TODO: stroke outline instead of fill when appropriate
            //aGal->SetIsFill( true );
            //aGal->DrawPolyline( &ptListScaled[0], ptCount );
            aGal->FillPolyline( ptListScaled );
        }

        cursor_x += ( pos.x_advance * advance_x_factor );
        cursor_y += ( pos.y_advance * advance_y_factor );
    }

    // Restore context
    aGal->Restore();
}


POINTS_LIST OUTLINE_FONT::outlineToStraightSegments( FT_Outline aOutline ) const
{
    POINTS_LIST contours;

    for( unsigned int c = 0; c < (unsigned int) aOutline.n_contours; c++ )
    {
        POINTS            bezier;
        POINTS            contour_points;
        std::vector<bool> contour_point_on_curve;
        char              prev_tags = 0;
        unsigned int      contour_start = 0;
        if( c > 0 )
        {
            contour_start = (unsigned int) ( aOutline.contours[c - 1] + 1 );
        }
        unsigned int contour_end = (unsigned int) aOutline.contours[c];
        FT_Vector    prev_point;
        prev_point.x = 0;
        prev_point.y = 0;

        for( unsigned int p = contour_start; p <= contour_end; p++ )
        {
            FT_Vector point = aOutline.points[p];
            char      tags = aOutline.tags[p];

#ifdef DEBUG
#ifdef FOOFAA
            std::cerr << "Point [" << point.x << "," << point.y << "] "
                      << ( onCurve( tags ) ? "on curve " : "off curve " )
                      << ( onCurve( tags )
                                   ? ""
                                   : ( thirdOrderBezierPoint( tags ) ? "3rd order Bezier point"
                                                                     : "2nd order Bezier point" ) );
            if( hasDropout( tags ) )
            {
                std::cerr << ", dropout " << dropoutMode( tags );
            }
            std::cerr << std::endl;
#endif
#endif
            if( thirdOrderBezierPoint( tags ) )
            {
                // TODO
                assert( 1 == 0 );
            }

            if( !onCurve( tags ) && !onCurve( prev_tags ) )
            {
                // Two consecutive off-curve control points, add virtual midpoint
                //
                // Note: 1st point of contour is assumed to be on-curve;
                // the font is malformed if this is not the case
                VECTOR2D midpoint( ( point.x + prev_point.x ) / 2.0,
                                   ( point.y + prev_point.y ) / 2.0 );
                contour_points.push_back( midpoint );
                contour_point_on_curve.push_back( true );
            }

            VECTOR2D pt( point.x, point.y );
            contour_points.push_back( pt );
            contour_point_on_curve.push_back( onCurve( tags ) ? true : false );
            prev_point = point;
            prev_tags = tags;
        }

        POINTS approximated_contour = approximateContour( contour_points, contour_point_on_curve );
        contours.push_back( approximated_contour );
    }

    return contours;
}


POINTS
OUTLINE_FONT::approximateContour( const POINTS&            contour_points,
                                  const std::vector<bool>& contour_point_on_curve ) const
{
    POINTS   approximation;
    POINTS   bezier;
    int      n = 0;
    VECTOR2D first_point;
    VECTOR2D prev_point;
    VECTOR2D prev2_point;
    bool     prev_on_curve;
    bool     prev2_on_curve;

    // note: we want to go past the end as end+1 will be a repeat of the 1st point!
    for( unsigned int i = 0; i <= contour_points.size(); i++ )
    {
        unsigned int nth = i < contour_points.size() ? i : 0;
        VECTOR2D     p = contour_points.at( nth );
        bool         on_curve = contour_point_on_curve.at( nth );
#ifdef DEBUG
#ifdef FOOFAA
        std::cerr << "Point " << p << " " << ( on_curve ? "on" : "off" ) << " curve" << std::endl;
#endif
#endif
        if( on_curve )
        {
            // This point is on curve
            if( n == 0 )
            {
                // First point
                approximation.push_back( p );
            }
            else
            {
                if( n > 0 && prev_on_curve )
                {
                    // Previous point is also on curve - straight line segment
                    approximation.push_back( p );
                }
                else
                {
                    // Previous point is not on curve
                    if( n > 1 )
                    {
                        if( prev2_on_curve )
                        {
                            // This point is on curve, previous is not, the one before that is
                            // == quadratic Bezier curve segment
                            //
                            // TODO: handle cubic curves
                            bezier.clear();
                            bezier.push_back( prev2_point );
                            bezier.push_back( prev_point );
                            bezier.push_back( p );
                            bool success = approximateBezierCurve( approximation, bezier );
                            if( !success )
                            {
                                std::cerr << "OUTLINE_FONT::approximateContour() Bezier curve "
                                             "approximation failed"
                                          << std::endl;
                            }
                        }
                        else
                        {
                            // This should not happen as the points have been
                            // preprocessed to insert implicit on-curve points!
                            std::cerr << "OUTLINE_FONT::approximateContour() impossible point "
                                         "sequence"
                                      << std::endl;
                            //assert( 1 == 0 );
                        }
                    }
                    else
                    {
                        // TODO: how did we get here?
                        std::cerr
                                << "OUTLINE_FONT::approximateContour() TODO define state, handling "
                                   "point #"
                                << n << std::endl;
                    }
                }
            }
        }

        n++;
        prev2_point = prev_point;
        prev_point = p;
        prev2_on_curve = prev_on_curve;
        prev_on_curve = on_curve;
    }

    return approximation;
}


// use converter in kimath
bool OUTLINE_FONT::approximateBezierCurve( POINTS& result, const POINTS& bezier ) const
{
    // cp0 = qp0, cp1 = qp0 + 2/3 * (qp1 - qp0), cp2 = qp2 + 2/3 * (qp1 - qp2), cp3 = qp2
    POINTS cubic;
    cubic.push_back( bezier.at( 0 ) );                                                     // cp0
    cubic.push_back( bezier.at( 0 ) + ( 2 / 3.0 ) * ( bezier.at( 1 ) - bezier.at( 0 ) ) ); // cp1
    cubic.push_back( bezier.at( 2 ) + ( 2 / 3.0 ) * ( bezier.at( 1 ) - bezier.at( 2 ) ) ); // cp2
    cubic.push_back( bezier.at( 2 ) );                                                     // cp3

    POINTS tmp;
    // BEZIER_POLY only handles cubic Bezier curves, even though the
    // comments say otherwise...
    BEZIER_POLY converter( cubic );
    // TODO: find out what the minimum segment length should really be!
    converter.GetPoly( tmp, 50 );

    for( unsigned int i = 0; i < tmp.size(); i++ )
        result.push_back( tmp.at( i ) );

    return true;
}
