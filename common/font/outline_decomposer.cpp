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

#include <font/outline_decomposer.h>
#include <bezier_curves.h>

OUTLINE_DECOMPOSER::OUTLINE_DECOMPOSER( FT_Outline& aOutline ) : m_outline( aOutline )
{
#ifdef DEBUG
    std::cerr << "[Cs n=" << aOutline.n_contours;
    for( int c = 0; c < aOutline.n_contours; c++ )
    {
        int contour_start = 0;
        if( c > 0 )
        {
            contour_start = aOutline.contours[c - 1] + 1;
        }
        int contour_end = aOutline.contours[c];

        std::cerr << "[c" << c << " " << contour_start << ".." << contour_end;

        for( int p = contour_start; p <= contour_end; p++ )
        {
            FT_Vector point = aOutline.points[p];
            FT_Pos    px = point.x;
            FT_Pos    py = point.y;
            char      tags = aOutline.tags[p];
            std::cerr << " [" << px << "," << py << "]";
        }
        std::cerr << " c" << c << "]\n";
    }
    std::cerr << "Cs]\n";
#endif
}


static VECTOR2D toVector2D( const FT_Vector* aFreeTypeVector )
{
#ifdef DEBUG
    std::cerr << "toVector2D() x " << aFreeTypeVector->x << " y " << aFreeTypeVector->y
              << std::endl;
#endif
    return VECTOR2D( aFreeTypeVector->x, aFreeTypeVector->y );
}


void OUTLINE_DECOMPOSER::contourToSegmentsAndArcs( CONTOUR&     aResult,
                                                   unsigned int aContourIndex ) const
{
    /*
      From https://www.freetype.org/freetype2/docs/glyphs/glyphs-6.html#section-1 :
      (note: conic == quadratic)

      The following rules are applied to decompose the contour's
      points into segments and arcs:

      Two successive ‘on’ points indicate a line segment joining them.

      One conic ‘off’ point between two ‘on’ points indicates a conic
      Bézier arc, the ‘off’ point being the control point, and the
      ‘on’ ones the start and end points.

      Two successive cubic ‘off’ points between two ‘on’ points
      indicate a cubic Bézier arc. There must be exactly two cubic
      control points and two ‘on’ points for each cubic arc (using a
      single cubic ‘off’ point between two ‘on’ points is forbidden,
      for example).

      Two successive conic ‘off’ points force the rasterizer to create
      (during the scan-line conversion process exclusively) a virtual
      ‘on’ point inbetween, at their exact middle. This greatly
      facilitates the definition of successive conic Bézier
      arcs. Moreover, it is the way outlines are described in the
      TrueType specification.

      The last point in a contour uses the first as an end point to
      create a closed contour. For example, if the last two points of
      a contour were an ‘on’ point followed by a conic ‘off’ point,
      the first point in the contour would be used as final point to
      create an ‘on’ – ‘off’ – ‘on’ sequence as described above.

      The first point in a contour can be a conic ‘off’ point itself;
      in that case, use the last point of the contour as the contour's
      starting point. If the last point is a conic ‘off’ point itself,
      start the contour with the virtual ‘on’ point between the last
      and first point of the contour.
    */

    POINTS            bezier;
    POINTS            contour_points;
    std::vector<bool> contour_point_on_curve;
    char              prev_tags = 0;
    unsigned int      contour_start = 0;
    if( aContourIndex > 0 )
    {
        contour_start = (unsigned int) ( m_outline.contours[aContourIndex - 1] + 1 );
    }
    unsigned int contour_end = (unsigned int) m_outline.contours[aContourIndex];
    FT_Vector    prev_point;
    prev_point.x = 0;
    prev_point.y = 0;

    for( unsigned int p = contour_start; p <= contour_end; p++ )
    {
        FT_Vector point = m_outline.points[p];
        char      tags = m_outline.tags[p];

        if( thirdOrderBezierPoint( tags ) )
        {
            // TODO! OpenType (Postscript) fonts can contain cubic Beziers!
            assert( 1 == 0 );
        }

        if( !onCurve( tags ) && !onCurve( prev_tags ) )
        {
            // Two consecutive off-curve control points, add virtual midpoint
            //
            // Note: 1st point of contour is assumed to be on-curve;
            // this may not be the case for all fonts!
            //
            // TODO: implement handling of first point as below
            //
            // From
            // https://www.freetype.org/freetype2/docs/glyphs/glyphs-6.html#section-1
            // :
            // The first point in a contour can be a conic ‘off’
            // point itself; in that case, use the last point of
            // the contour as the contour's starting point. If the
            // last point is a conic ‘off’ point itself, start the
            // contour with the virtual ‘on’ point between the
            // last and first point of the contour.
            VECTOR2D midpoint( ( point.x + prev_point.x ) / 2.0, ( point.y + prev_point.y ) / 2.0 );
            contour_points.push_back( midpoint );
            contour_point_on_curve.push_back( true );
        }

        VECTOR2D pt( point.x, point.y );
        contour_points.push_back( pt );
        contour_point_on_curve.push_back( onCurve( tags ) ? true : false );
        prev_point = point;
        prev_tags = tags;
    }

    aResult.winding = approximateContour( contour_points, contour_point_on_curve, aResult.points );
}


void OUTLINE_DECOMPOSER::newContour()
{
    CONTOUR contour;
    contour.orientation = FT_Outline_Get_Orientation( &m_outline );
    m_contours->push_back( contour );
}


void OUTLINE_DECOMPOSER::addContourPoint( const VECTOR2D& p )
{
    // don't add repeated points
    if( m_contours->back().points.empty() || m_contours->back().points.back() != p )
    {
#ifdef DEBUG
        std::cerr << "OUTLINE_DECOMPOSER::addContourPoint( " << p << " )\n";
#endif
        m_contours->back().points.push_back( p );
    }
}


int OUTLINE_DECOMPOSER::moveTo( const FT_Vector* aEndPoint, void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

#ifdef DEBUG
    FT_Pos x = aEndPoint->x;
    FT_Pos y = aEndPoint->y;
    std::cerr << "moveTo([ " << x << "," << y << " ]) ";
#endif

    decomposer->m_lastEndPoint.x = aEndPoint->x;
    decomposer->m_lastEndPoint.y = aEndPoint->y;

    decomposer->newContour();
    decomposer->addContourPoint( decomposer->m_lastEndPoint );

    return 0;
}


int OUTLINE_DECOMPOSER::lineTo( const FT_Vector* aEndPoint, void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

#ifdef DEBUG
    FT_Pos x = aEndPoint->x;
    FT_Pos y = aEndPoint->y;
    std::cerr << "lineTo([ " << x << "," << y << " ]) ";
#endif

    decomposer->m_lastEndPoint.x = aEndPoint->x;
    decomposer->m_lastEndPoint.y = aEndPoint->y;

    decomposer->addContourPoint( decomposer->m_lastEndPoint );

    return 0;
}


int OUTLINE_DECOMPOSER::quadraticTo( const FT_Vector* aControlPoint, const FT_Vector* aEndPoint,
                                     void* aCallbackData )
{
    return cubicTo( aControlPoint, nullptr, aEndPoint, aCallbackData );
}


int OUTLINE_DECOMPOSER::cubicTo( const FT_Vector* aFirstControlPoint,
                                 const FT_Vector* aSecondControlPoint, const FT_Vector* aEndPoint,
                                 void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

#ifdef DEBUG
    std::cerr << "cubicTo( ";
    FT_Pos x = aFirstControlPoint->x;
    FT_Pos y = aFirstControlPoint->y;
    std::cerr << "[" << x << "," << y << "],";
    if( aSecondControlPoint )
    {
        x = aSecondControlPoint->x;
        y = aSecondControlPoint->y;
        std::cerr << "[" << x << "," << y << "],";
    }
    else
    {
        std::cerr << "null,";
    }
    x = aEndPoint->x;
    y = aEndPoint->y;
    std::cerr << "[" << x << "," << y << "] ) ";
#endif
    /* ... figure out what we're starting from if lastEndPoint is not set */

    POINTS result;
    POINTS bezier;
    bezier.push_back( decomposer->m_lastEndPoint );
    bezier.push_back( toVector2D( aFirstControlPoint ) );
    if( aSecondControlPoint )
    {
        // aSecondControlPoint == nullptr for quadratic Beziers
        bezier.push_back( toVector2D( aSecondControlPoint ) );
    }
    bezier.push_back( toVector2D( aEndPoint ) );
    decomposer->approximateBezierCurve( result, bezier );

    for( VECTOR2D p : result )
    {
        // store result
        decomposer->addContourPoint( p );
    }

    decomposer->m_lastEndPoint.x = aEndPoint->x;
    decomposer->m_lastEndPoint.y = aEndPoint->y;

    return 0;
}


void OUTLINE_DECOMPOSER::OutlineToSegments( CONTOURS* aContours )
{
    m_contours = aContours;

#if 0
    FT_Orientation orientation = FT_Outline_Get_Orientation( &m_outline );

    for( unsigned int c = 0; c < (unsigned int) m_outline.n_contours; c++ )
    {
        CONTOUR contour;
        contour.orientation = orientation;
        contourToSegmentsAndArcs( contour, c );
        m_contours->push_back( contour );
    }
#else
    FT_Outline_Funcs callbacks;

    callbacks.move_to = moveTo;
    callbacks.line_to = lineTo;
    callbacks.conic_to = quadraticTo;
    callbacks.cubic_to = cubicTo;
    callbacks.shift = 0;
    callbacks.delta = 0;

    FT_Error e = FT_Outline_Decompose( &m_outline, &callbacks, this );
    if( e )
    {
        // TODO: handle error != 0
    }

    for( CONTOUR& c : *m_contours )
    {
        c.winding = winding( c.points );
    }
#endif
}


int OUTLINE_DECOMPOSER::approximateContour( const POINTS&            aPoints,
                                            const std::vector<bool>& aPointOnCurve,
                                            POINTS&                  aResult ) const
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
bool OUTLINE_DECOMPOSER::approximateQuadraticBezierCurve( POINTS&       aResult,
                                                          const POINTS& aQuadraticBezier ) const
{
    // TODO: assert aQuadraticBezier.size == 3

    // BEZIER_POLY only handles cubic Bezier curves, even though the
    // comments say otherwise...
    //
    // Quadratic to cubic Bezier conversion:
    // cpn = Cubic Bezier control points (n = 0..3, 4 in total)
    // qpn = Quadratic Bezier control points (n = 0..2, 3 in total)
    // cp0 = qp0, cp1 = qp0 + 2/3 * (qp1 - qp0), cp2 = qp2 + 2/3 * (qp1 - qp2), cp3 = qp2

    POINTS cubic;

    cubic.push_back( aQuadraticBezier.at( 0 ) ); // cp0
    cubic.push_back( aQuadraticBezier.at( 0 )
                     + ( 2 / 3.0 )
                               * ( aQuadraticBezier.at( 1 ) - aQuadraticBezier.at( 0 ) ) ); // cp1
    cubic.push_back( aQuadraticBezier.at( 2 )
                     + ( 2 / 3.0 )
                               * ( aQuadraticBezier.at( 1 ) - aQuadraticBezier.at( 2 ) ) ); // cp2
    cubic.push_back( aQuadraticBezier.at( 2 ) );                                            // cp3

    return approximateCubicBezierCurve( aResult, cubic );
}


bool OUTLINE_DECOMPOSER::approximateCubicBezierCurve( POINTS&       aResult,
                                                      const POINTS& aCubicBezier ) const
{
    // TODO: assert aCubicBezier.size == 4

    // TODO: find out what the minimum segment length should really be!
    static const int minimumSegmentLength = 50;
    POINTS           tmp;
    BEZIER_POLY      converter( aCubicBezier );
    converter.GetPoly( tmp, minimumSegmentLength );

    for( unsigned int i = 0; i < tmp.size(); i++ )
        aResult.push_back( tmp.at( i ) );

    return true;
}


bool OUTLINE_DECOMPOSER::approximateBezierCurve( POINTS& aResult, const POINTS& aBezier ) const
{
    bool bezierIsCubic = ( aBezier.size() == 4 );

    if( bezierIsCubic )
    {
        return approximateCubicBezierCurve( aResult, aBezier );
    }
    else
    {
        return approximateQuadraticBezierCurve( aResult, aBezier );
    }
}


int OUTLINE_DECOMPOSER::winding( const POINTS& aContour ) const
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
