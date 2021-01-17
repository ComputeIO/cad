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
#include <bezier_curves.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_poly_set.h>

using namespace KIGFX;

FT_Library OUTLINE_FONT::mFreeType = nullptr;

OUTLINE_FONT::OUTLINE_FONT() : mReferencedFont( nullptr )
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
void OUTLINE_FONT::Draw( GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
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

    if( !mReferencedFont )
    {
        mReferencedFont = hb_ft_font_create_referenced( mFace );
        hb_ft_font_set_funcs( mReferencedFont );
    }

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


void OUTLINE_FONT::drawSingleLineText( GAL* aGal, const UTF8& aText )
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

    hb_shape( mReferencedFont, buf, NULL, 0 );

    const double mirror_factor = ( aIsMirrored ? 1 : -1 );
    // TODO: xyscaler is determined with the Stetson method to make
    // debugging easier; root cause for why scaling does not seem to
    // work is unknown (but most likely trivial)
    const double xyscaler = 2.0e3;
    const double x_scale_factor = mirror_factor * aGlyphSize.x / xyscaler;
    const double y_scale_factor = aGlyphSize.y / xyscaler;
    // TODO: why 2x? not sure why this is needed, but without it
    // advance (at least X advance, haven't yet witnessed non-zero Y
    // advance) seems to be about 0.5x what is required
    const double advance_scale_factor = 2;
    const double advance_x_factor = advance_scale_factor;
    const double advance_y_factor = advance_scale_factor;

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;

    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        hb_glyph_position_t& pos = glyphPos[i];
        int                  codepoint = glyphInfo[i].codepoint;

        FT_Load_Glyph( mFace, codepoint, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE );

        FT_GlyphSlot       glyph = mFace->glyph;
        const unsigned int bufsize = 512;
        char               glyphName[bufsize];
        FT_Get_Glyph_Name( mFace, glyph->glyph_index, &glyphName[0], bufsize );

        // contours is a collection of all outlines in the glyph
        // example: glyph for 'o' generally contains 2 contours,
        // one for the glyph outline and one for the hole
        //
        // might be a good idea to cache the contours,
        // in which case glyph->glyph_index can be used as a std::map key
        CONTOURS contours;
        outlineToStraightSegments( contours, glyph->outline );

        SHAPE_POLY_SET             poly;
        std::vector<SHAPE_SIMPLE*> holes;
        std::vector<SHAPE_SIMPLE*> outlines;
        std::vector<VECTOR2D>      ptListScaled;

        for( CONTOUR c : contours )
        {
            POINTS points = c.points;
            int ptCount = 0;
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

#ifdef FOO // DEBUG
    std::cerr << "GetTextAsPolygon() \"" << aText << "\" glyph size {" << aGlyphSize.x << ","
              << aGlyphSize.y << "}" << ( aIsMirrored ? " mirrored" : "" ) << " " << aGlyphs.size()
              << " glyphs.";
    for( unsigned int foo = 0; foo < aGlyphs.size(); foo++ )
    {
        const SHAPE_POLY_SET& glyph = aGlyphs[foo];
        int                   codepoint = glyphInfo[foo].codepoint;

        FT_Load_Glyph( mFace, codepoint, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE );

        FT_GlyphSlot       glyphSlot = mFace->glyph;
        const unsigned int bufsize = 512;
        char               glyphName[bufsize];
        FT_Get_Glyph_Name( mFace, glyphSlot->glyph_index, &glyphName[0], bufsize );

        std::cerr << "Glyph " << foo << " [" << glyphSlot->glyph_index << " " << glyphName
                  << "] has " << glyph.OutlineCount() << " outlines. ";
        for( int bar = 0; bar < glyph.OutlineCount(); bar++ )
        {
            const SHAPE_LINE_CHAIN& outline = glyph.COutline( bar );
            std::cerr << " O" << bar << "; " << outline.PointCount() << "p, "
                      << glyph.HoleCount( bar ) << "h.";
            for( int iHole = 0; iHole < glyph.HoleCount( bar ); iHole++ )
            {
                const SHAPE_LINE_CHAIN& hole = glyph.CHole( bar, iHole );
                std::cerr << " H" << iHole << ": " << hole.PointCount() << "p.";
            }
        }
        std::cerr << std::endl;
    }
#endif
}


void OUTLINE_FONT::outlineToStraightSegments( CONTOURS& aContours, FT_Outline& aOutline ) const
{
    FT_Orientation orientation = FT_Outline_Get_Orientation( &aOutline );

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

            if( thirdOrderBezierPoint( tags ) )
            {
                // TODO! Some fonts contain cubic Beziers!
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

        CONTOUR theContour;
        theContour.winding =
                approximateContour( contour_points, contour_point_on_curve, theContour.points );
        theContour.orientation = orientation;
        aContours.push_back( theContour );
    }
}


int OUTLINE_FONT::winding( const POINTS& aContour ) const
{
    // -1 == counterclockwise, 1 == clockwise

    const int cw = 1;
    const int ccw = -1;

    if( aContour.size() < 2 )
    {
        // zero or one points, so not a clockwise contour - in fact
        // not a contour at all
        //
        // It could also be argued that a contour needs 3 extremum
        // points at a minimum to be considered a proper contour
        // (ie. a glyph (subpart) outline, or a hole)
        return 0;
    }

    unsigned int i_lowest_vertex;
    double       lowest_y = std::numeric_limits<double>::max();

    for( unsigned int i = 0; i < aContour.size(); i++ )
    {
        VECTOR2D p = aContour[i];
        if( p.y < lowest_y )
        {
            i_lowest_vertex = i;
            lowest_y = p.y;

            // note: we should also check for p.y == lowest_y and
            // then choose the point with leftmost.x, but as p.x
            // is a double, equality is a dubious concept; however
            // this should suffice in the general case
        }
    }

    unsigned int i_prev_vertex;
    unsigned int i_next_vertex;
    // TODO: this should be done with modulo arithmetic for clarity
    if( i_lowest_vertex == 0 )
    {
        i_prev_vertex = aContour.size() - 1;
    }
    else
    {
        i_prev_vertex = i_lowest_vertex - 1;
    }
    if( i_lowest_vertex == aContour.size() - 1 )
    {
        i_next_vertex = 0;
    }
    else
    {
        i_next_vertex = i_lowest_vertex + 1;
    }

    const VECTOR2D& lowest = aContour[i_lowest_vertex];
    VECTOR2D        prev( aContour[i_prev_vertex] );
    while( prev == lowest )
    {
        if( i_prev_vertex == 0 )
        {
            i_prev_vertex = aContour.size() - 1;
        }
        else
        {
            i_prev_vertex--;
        }
        if( i_prev_vertex == i_lowest_vertex )
        {
            // ERROR: degenerate contour (all points are equal)
            // TODO: signal error
            // for now let's just return something at random
            return cw;
        }
        prev = aContour[i_prev_vertex];
    }
    VECTOR2D next( aContour[i_next_vertex] );
    while( next == lowest )
    {
        if( i_next_vertex == aContour.size() - 1 )
        {
            i_next_vertex = 0;
        }
        else
        {
            i_next_vertex++;
        }
        if( i_next_vertex == i_lowest_vertex )
        {
            // ERROR: degenerate contour (all points are equal)
            // TODO: signal error
            // for now let's just return something at random
            return cw;
        }
        next = aContour[i_next_vertex];
    }

    // winding is figured out based on the angle between the lowest
    // vertex and its neighbours
    //
    // prev.x < lowest.x && next.x > lowest.x -> ccw
    //
    // prev.x > lowest.x && next.x < lowest.x -> cw
    //
    // prev.x < lowest.x && next.x < lowest.x:
    // ?
    //
    // prev.x > lowest.x && next.x > lowest.x:
    // ?
    //
    if( prev.x < lowest.x && next.x > lowest.x )
        return ccw;

    if( prev.x > lowest.x && next.x < lowest.x )
        return cw;

    double prev_deltaX = prev.x - lowest.x;
    double prev_deltaY = prev.y - lowest.y;
    double next_deltaX = next.x - lowest.x;
    double next_deltaY = next.y - lowest.y;

    double prev_atan = atan2( prev_deltaY, prev_deltaX );
    double next_atan = atan2( next_deltaY, next_deltaX );

    if( prev_atan > next_atan )
        return ccw;
    else
        return cw;
}


int OUTLINE_FONT::approximateContour( const POINTS& aPoints, const std::vector<bool>& aPointOnCurve,
                                      POINTS& aResult ) const
{
    POINTS   bezier;
    int      n = 0;
    VECTOR2D first_point;
    VECTOR2D prev_point;
    VECTOR2D prev2_point;
    bool     prev_on_curve;
    bool     prev2_on_curve;

    // note: we want to go past the end as end+1 will be a repeat of the 1st point!
    for( unsigned int i = 0; i <= aPoints.size(); i++ )
    {
        unsigned int nth = i < aPoints.size() ? i : 0;
        VECTOR2D     p = aPoints.at( nth );
        bool         on_curve = aPointOnCurve.at( nth );

        if( on_curve )
        {
            if( n == 0 )
            {
                // First point
                aResult.push_back( p );
            }
            else
            {
                if( n > 0 && prev_on_curve )
                {
                    // Previous point is also on curve - straight line segment
                    aResult.push_back( p );
                }
                else
                {
                    // Previous point is a control point not on curve
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
                            bool success = approximateBezierCurve( aResult, bezier );
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
                        }
                    }
                    else
                    {
                        // How did we get here?
                        std::cerr << "OUTLINE_FONT::approximateContour() undefined state "
                                     "processing point #"
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

    // it's probably possible to figure out winding here without
    // another traversal of all vertexes, doing that for now though
    return winding( aResult );
}


// use converter in kimath
bool OUTLINE_FONT::approximateBezierCurve( POINTS& result, const POINTS& bezier ) const
{
    // Quadratic to cubic Bezier conversion:
    // cpn = Cubic Bezier control points (n = 0..3, 4 in total)
    // qpn = Quadratic Bezier control points (n = 0..2, 3 in total)
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
