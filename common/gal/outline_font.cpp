/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2016 Kicad Developers, see change_log.txt for contributors.
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
                         double aRotationAngle ) const
{
    if( aText.empty() )
        return;

    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, aText.c_str(), -1, 0, -1 );

    hb_buffer_set_direction( buf, HB_DIRECTION_LTR );
    hb_buffer_set_script( buf, HB_SCRIPT_LATIN );
    hb_buffer_set_language( buf, hb_language_from_string( "en", -1 ) );

    FT_Select_Charmap( mFace, FT_Encoding::FT_ENCODING_UNICODE );
    FT_Set_Char_Size( mFace, 0, 1000, 0, 0 );

#if 0
    hb_blob_t* blob = hb_blob_create_from_file( m_fontFileName.c_str() );
    hb_face_t* face = hb_face_create( blob, 0 );
    hb_font_t* font = hb_font_create( face );
#else
    hb_font_t* font = hb_ft_font_create_referenced( mFace );
    hb_ft_font_set_funcs( font );
#endif

    hb_shape( font, buf, NULL, 0 );

    unsigned int         glyph_count;
    hb_glyph_info_t*     glyph_info = hb_buffer_get_glyph_infos( buf, &glyph_count );
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions( buf, &glyph_count );

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;
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

    // Context needs to be saved before any transformations
    aGal->Save();

    aGal->Translate( aPosition );
    aGal->Rotate( -aRotationAngle );
    const VECTOR2D& glyphSize = aGal->GetGlyphSize();

    std::cerr << "GAL glyphSize == " << glyphSize << std::endl;

#ifdef FOOFAA
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

    m_gal->SetIsStroke( true );
    //m_gal->SetIsFill( false );

    if( m_gal->IsFontBold() )
    {
        // TODO: figure out how to do real bold
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

#endif // FOOFAA

    drawSingleLineText( aGal, buf, font );

    hb_buffer_destroy( buf );
#if 0
    hb_font_destroy( font );
    hb_face_destroy( face );
    hb_blob_destroy( blob );
#endif
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


void OUTLINE_FONT::drawSingleLineText( GAL* aGal, hb_buffer_t* aText, hb_font_t* aFont ) const
{
    const VECTOR2D& galGlyphSize = aGal->GetGlyphSize();

    // Context needs to be saved before any transformations
    aGal->Save();

    unsigned int         glyphCount;
    hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( aText, &glyphCount );
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( aText, &glyphCount );

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;
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

    const int             GLYPH_NAME_LEN = 256;
    char                  glyph_name[GLYPH_NAME_LEN];
    std::vector<VECTOR2D> ptListScaled;

    const double mirror_factor = ( aGal->IsTextMirrored() ? 1 : -1 );
    const double xyscaler = 3.0e2;
    const double x_scale_factor = mirror_factor * galGlyphSize.x / xyscaler;
    const double y_scale_factor = galGlyphSize.y / xyscaler;
    const double advance_scale_factor = 1.5; // / 10.0;

    cursor_x = 0;
    cursor_y = 0;
    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        int                  cluster = glyphInfo[i].cluster;
        int                  codepoint = glyphInfo[i].codepoint;
        hb_glyph_position_t& pos = glyphPos[i];
        std::cerr << "ch[" << i << "] cluster " << cluster << " codepoint " << codepoint
                  << " advance " << pos.x_advance << "," << pos.y_advance << " offset "
                  << pos.x_offset << "," << pos.y_offset << std::endl;

        FT_Load_Glyph( mFace, codepoint, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE );
        FT_Get_Glyph_Name( mFace, codepoint, glyph_name, GLYPH_NAME_LEN );

        FT_GlyphSlot glyph = mFace->glyph;
        unsigned int glyph_index = glyph->glyph_index;

#ifdef DEBUG
        std::cerr << "glyph name [" << glyph_name << "] " << glyph_index << std::endl;
#endif

        POINTS_LIST contours = outlineToStraightSegments( glyph->outline );
        for( POINTS points : contours )
        {
            int ptCount = 0;
            ptListScaled.clear();
            VECTOR2D firstPoint;
            for( VECTOR2D v : points )
            {
                VECTOR2D pt( -( v.x + cursor_x ), -( v.y + cursor_y ) );
                VECTOR2D scaledPt( pt.x * x_scale_factor, pt.y * y_scale_factor );
                ptListScaled.push_back( scaledPt );
                if( ptCount == 0 )
                {
                    // save 1st point as we need to draw a line back to it at the end
                    firstPoint = scaledPt;
                }
                ptCount++;
            }

            if( !ptListScaled.empty() )
            {
                aGal->DrawPolyline( &ptListScaled[0], ptCount );
            }
        }

        cursor_x += ( pos.x_advance * advance_scale_factor );
        cursor_y += ( pos.y_advance * advance_scale_factor );
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
    POINTS                            approximation;
    POINTS                            bezier;
    POINTS::const_iterator            point_it = contour_points.begin();
    POINTS::const_iterator            prev_point_it;
    POINTS::const_iterator            prev2_point_it;
    std::vector<bool>::const_iterator on_curve_it = contour_point_on_curve.begin();
    std::vector<bool>::const_iterator prev_on_curve_it;
    std::vector<bool>::const_iterator prev2_on_curve_it;
    int                               n = 0;
    VECTOR2D                          first_point = *point_it;
    while( point_it != contour_points.end() )
    {
#ifdef DEBUG
        VECTOR2D p = *point_it;
        std::cerr << "Point " << p << " " << ( *on_curve_it ? "on" : "off" ) << " curve"
                  << std::endl;
#endif
        if( *on_curve_it )
        {
            // This point is on curve
            if( n == 0 )
            {
                // First point
                approximation.push_back( *point_it );
            }
            else
            {
                if( n > 0 && *prev_on_curve_it )
                {
                    // Previous point is also on curve - straight line segment
                    approximation.push_back( *point_it );
                }
                else
                {
                    // Previous point is not on curve
                    if( n > 1 )
                    {
                        if( *prev2_on_curve_it )
                        {
                            // This point is on curve, previous is not, the one before that is
                            // == quadratic Bezier curve segment
                            //
                            // TODO: handle cubic curves
                            bezier.clear();
                            bezier.push_back( *prev2_point_it );
                            bezier.push_back( *prev_point_it );
                            bezier.push_back( *point_it );
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
        prev2_point_it = prev_point_it;
        prev_point_it = point_it++;
        prev2_on_curve_it = prev_on_curve_it;
        prev_on_curve_it = on_curve_it++;
    }

    // add a copy of 1st point to end to close the contour
    approximation.push_back( first_point );
    return approximation;
}


VECTOR2D OUTLINE_FONT::approximateBezierPoint( const double aT, const double aOneMinusT,
                                               const VECTOR2D& aPt1, const VECTOR2D& aPt2 ) const
{
    VECTOR2D pt1prime( aPt1.x * aOneMinusT, aPt1.y * aOneMinusT );
    VECTOR2D pt2prime( aPt2.x * aT, aPt2.y * aT );
    VECTOR2D r = pt1prime + pt2prime;
    return r;
}


bool OUTLINE_FONT::approximateBezierCurve( POINTS& result, const POINTS& bezier ) const
{
    std::vector<VECTOR2D> approximatedPoints;

    const double precision = 0.1;
    for( double t = 0; t < 1; t += precision )
    {
        double oneMinusT = ( 1 - t );
        approximatedPoints = bezier;
        while( approximatedPoints.size() > 1 )
        {
            auto ptIter = approximatedPoints.begin();
            auto ptIterEnd = std::prev( approximatedPoints.end() );
            while( ptIter != ptIterEnd )
            {
                auto        updateThisIter = ptIter;
                const auto& pt1 = *( ptIter++ );
                const auto& pt2 = *( ptIter );
                const auto  approximatedPoint = approximateBezierPoint( t, oneMinusT, pt1, pt2 );
                *updateThisIter = approximatedPoint;
                approximatedPoints.pop_back();
            }
            result.push_back( approximatedPoints[0] );
        }
    }
    return true;
}
