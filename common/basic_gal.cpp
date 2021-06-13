/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @brief Implement a very basic GAL used to draw, plot and convert texts in segments
 * for DRC functions, using the common GAL functions.
 * Draw functions use wxDC.
 * Plot functions use a PLOTTER
 * Convert texts in segments use a callback function created by the caller
 * @file basic_gal.cpp
 */

#include <gr_basic.h>
#include <plotter.h>
#include <trigo.h>
#include <geometry/shape_poly_set.h>
#include <font/triangulate.h>

#include <basic_gal.h>

using namespace KIGFX;

KIGFX::GAL_DISPLAY_OPTIONS basic_displayOptions;

// the basic GAL doesn't get an external display option object
BASIC_GAL basic_gal( basic_displayOptions );


const VECTOR2D BASIC_GAL::transform( const VECTOR2D& aPoint ) const
{
    VECTOR2D point = aPoint + m_transform.m_moveOffset - m_transform.m_rotCenter;
    point = point.Rotate( m_transform.m_rotAngle ) + m_transform.m_rotCenter;
    return point;
}


// Draws a polyline given a list of points already transformed into the local coordinate
// system.
void BASIC_GAL::doDrawPolyline( const std::vector<wxPoint>& aLocalPointList, bool aFill )
{
#ifdef DEBUG
    std::cerr << "BASIC_GAL::doDrawPolyline( ..., " << ( aFill ? "t" : "f" ) << " ) "
              << aLocalPointList.size() << " pts, "
              << ( m_DC ? ( m_isFillEnabled ? "DC fill " : "DC no_fill " ) : "!DC " )
              << ( m_plotter ? "plotter " : "!plotter " )
              << ( m_callback ? "callback " : "!callback " ) << std::endl;
#endif
    if( m_DC )
    {
        if( m_isFillEnabled )
        {
            GRPoly( m_isClipped ? &m_clipBox : NULL, m_DC, aLocalPointList.size(),
                    &aLocalPointList[0], 0, 0, m_Color, m_Color );
        }
        else
        {
            for( unsigned ii = 1; ii < aLocalPointList.size(); ++ii )
            {
                GRCSegm( m_isClipped ? &m_clipBox : NULL, m_DC, aLocalPointList[ii - 1],
                         aLocalPointList[ii], GetLineWidth(), m_Color );
            }
        }
    }
    else if( m_plotter )
    {
        if( aFill )
        {
            m_plotter->PlotPoly( aLocalPointList, FILL_TYPE::FILLED_SHAPE );
        }
        else
        {
            m_plotter->MoveTo( aLocalPointList[0] );

            for( unsigned ii = 1; ii < aLocalPointList.size(); ii++ )
            {
                m_plotter->LineTo( aLocalPointList[ii] );
            }
        }
        m_plotter->PenFinish();
    }
    else if( m_callback )
    {
        for( unsigned ii = 1; ii < aLocalPointList.size(); ii++ )
        {
            m_callback( aLocalPointList[ii - 1].x, aLocalPointList[ii - 1].y, aLocalPointList[ii].x,
                        aLocalPointList[ii].y, m_callbackData );
        }
    }
}


void BASIC_GAL::DrawPolyline( const std::deque<VECTOR2D>& aPointList )
{
    if( aPointList.size() < 2 )
        return;

    std::vector<wxPoint> polyline_corners;

    for( const VECTOR2D& pt : aPointList )
        polyline_corners.emplace_back( (wxPoint) transform( pt ) );

    doDrawPolyline( polyline_corners );
}


void BASIC_GAL::DrawPolyline( const VECTOR2D aPointList[], int aListSize )
{
    if( aListSize < 2 )
        return;

    std::vector<wxPoint> polyline_corners;

    for( int ii = 0; ii < aListSize; ++ii )
        polyline_corners.emplace_back( (wxPoint) transform( aPointList[ii] ) );

    doDrawPolyline( polyline_corners );
}


void BASIC_GAL::DrawPolyline( const SHAPE_LINE_CHAIN& aLineChain )
{
#ifdef DEBUG
    std::cerr << "BASIC_GAL::DrawPolyline( " << aLineChain << " ";
    if( m_DC )
        std::cerr << "DC ";
    if( m_isFillEnabled )
        std::cerr << "fill ";
    if( m_plotter )
        std::cerr << "plotter ";
    if( m_callback )
        std::cerr << "callback ";
    std::cerr << " )" << std::endl;
#endif
    if( aLineChain.PointCount() < 2 )
        return;

    std::vector<wxPoint> polyline_corners;

    for( int i = 0; i < aLineChain.PointCount(); i++ )
    {
        polyline_corners.emplace_back( (wxPoint) transform( aLineChain.CPoint( i ) ) );
    }

    if( aLineChain.IsClosed() )
        polyline_corners.emplace_back( (wxPoint) transform( aLineChain.CPoint( 0 ) ) );

    doDrawPolyline( polyline_corners );
}


void BASIC_GAL::DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    VECTOR2D startVector = transform( aStartPoint );
    VECTOR2D endVector = transform( aEndPoint );

    if( m_DC )
    {
        if( m_isFillEnabled )
        {
            GRLine( m_isClipped ? &m_clipBox : NULL, m_DC, startVector.x, startVector.y,
                    endVector.x, endVector.y, GetLineWidth(), m_Color );
        }
        else
        {
            GRCSegm( m_isClipped ? &m_clipBox : NULL, m_DC, startVector.x, startVector.y,
                     endVector.x, endVector.y, GetLineWidth(), 0, m_Color );
        }
    }
    else if( m_plotter )
    {
        m_plotter->MoveTo( wxPoint( startVector.x, startVector.y ) );
        m_plotter->LineTo( wxPoint( endVector.x, endVector.y ) );
        m_plotter->PenFinish();
    }
    else if( m_callback )
    {
        m_callback( startVector.x, startVector.y, endVector.x, endVector.y, m_callbackData );
    }
}


void BASIC_GAL::DrawGlyph( const SHAPE_POLY_SET& aPolySet, int aNth, int aTotal )
{
#ifdef DEBUG
    std::cerr << "BASIC_GAL::DrawGlyph( [aPolySet], " << aNth << ", " << aTotal << " ) ";
    if (m_plotter)
        std::cerr << m_plotter->GetPlotterType();
    else
        std::cerr << "!plotter";
    std::cerr << ( m_DC ? " DC " : " !DC " ) << std::endl;
#endif
    if( m_plotter )
    {
        switch( m_plotter->GetPlotterType() )
        {
        case PLOT_FORMAT::GERBER:
        {
            // TODO move this to GERBER_plotter.cpp
            std::vector<wxPoint> polygon_with_transform;
            int                  i;

            for( int iOutline = 0; iOutline < aPolySet.OutlineCount(); ++iOutline )
            {
                const SHAPE_LINE_CHAIN& outline = aPolySet.COutline( iOutline );

                if( outline.PointCount() < 2 )
                    continue;

                polygon_with_transform.clear();

                for( i = 0; i < outline.PointCount(); i++ )
                    polygon_with_transform.emplace_back(
                            (wxPoint) transform( outline.CPoint( i ) ) );

                m_plotter->SetLayerPolarity( true );
                m_plotter->PlotPoly( polygon_with_transform, FILL_TYPE::FILLED_SHAPE );

                for( int iHole = 0; iHole < aPolySet.HoleCount( iOutline ); iHole++ )
                {
                    const SHAPE_LINE_CHAIN& hole = aPolySet.CHole( iOutline, iHole );

                    if( hole.PointCount() < 2 )
                        continue;

                    polygon_with_transform.clear();

                    for( i = 0; i < hole.PointCount(); i++ )
                        polygon_with_transform.emplace_back(
                                (wxPoint) transform( hole.CPoint( i ) ) );

                    // Note: holes in glyphs are erased, they are not see-thru
                    m_plotter->SetLayerPolarity( false );
                    m_plotter->PlotPoly( polygon_with_transform, FILL_TYPE::FILLED_SHAPE );
                }

                m_plotter->SetLayerPolarity( true );
                m_plotter->PenFinish();
            }
        }
        break;

        case PLOT_FORMAT::POST:
        case PLOT_FORMAT::PDF:
        case PLOT_FORMAT::DXF:
        case PLOT_FORMAT::HPGL:
        case PLOT_FORMAT::SVG:
        {
            // TODO move this to the appropriate plotter (PDF?) class
            auto triangleCallback = [&]( int aPolygonIndex, const VECTOR2D& aVertex1,
                                         const VECTOR2D& aVertex2, const VECTOR2D& aVertex3,
                                         void* aCallbackData )
            {
                std::vector<wxPoint> corners;
                corners.emplace_back( (wxPoint) transform( aVertex1 ) );
                corners.emplace_back( (wxPoint) transform( aVertex2 ) );
                corners.emplace_back( (wxPoint) transform( aVertex3 ) );
                m_plotter->PlotPoly( corners, FILL_TYPE::FILLED_SHAPE, 0 );
            };

            // foo
            Triangulate( aPolySet, triangleCallback );
            m_plotter->PenFinish();
        }
        break;

        default:
            // TODO: some sort of error notification might be handy
            break;
        }
    }
    else
    {
#if 1
        const wxBrush& saveBrush = m_DC->GetBrush();

        for( int iOutline = 0; iOutline < aPolySet.OutlineCount(); ++iOutline )
        {
            const SHAPE_LINE_CHAIN& outline = aPolySet.COutline( iOutline );
            std::vector<wxPoint>    outline_with_transform;

            for( int i = 0; i < outline.PointCount(); i++ )
                outline_with_transform.emplace_back( (wxPoint) transform( outline.CPoint( i ) ) );

            m_DC->SetBrush( saveBrush );
            doDrawPolyline( outline_with_transform, true );

            m_DC->SetBrush( *wxWHITE_BRUSH );
            for( int iHole = 0; iHole < aPolySet.HoleCount( iOutline ); iHole++ )
            {
                const SHAPE_LINE_CHAIN& hole = aPolySet.CHole( iOutline, iHole );
                std::vector<wxPoint>    hole_with_transform;
                for( int i = 0; i < hole.PointCount(); i++ )
                    hole_with_transform.emplace_back( (wxPoint) transform( hole.CPoint( i ) ) );

                doDrawPolyline( hole_with_transform, true );
            }
        }

        m_DC->SetBrush( saveBrush );
#else
        // TODO move this to the appropriate plotter (PDF?) class
        auto triangleCallback = [&]( int aPolygonIndex, const VECTOR2D& aVertex1,
                                     const VECTOR2D& aVertex2, const VECTOR2D& aVertex3,
                                     void* aCallbackData )
        {
            std::vector<wxPoint> corners;
            corners.emplace_back( (wxPoint) transform( aVertex1 ) );
            corners.emplace_back( (wxPoint) transform( aVertex2 ) );
            corners.emplace_back( (wxPoint) transform( aVertex3 ) );
            // m_plotter->PlotPoly( corners, FILL_TYPE::FILLED_SHAPE, 0 );
            doDrawPolyline( corners, true );
        };

        if( m_DC )
        {
            const wxPen& savePen = m_DC->GetPen();
            const wxBrush& saveBrush = m_DC->GetBrush();
            m_DC->SetPen( wxNullPen );
            // TODO: support other colors
            m_DC->SetBrush( *wxBLACK_BRUSH );
            Triangulate( aPolySet, triangleCallback );
            m_DC->SetPen( savePen );
            m_DC->SetBrush( saveBrush );
        } else {
            Triangulate( aPolySet, triangleCallback );
        }
#endif
    }
}
