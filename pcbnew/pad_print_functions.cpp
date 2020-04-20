/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <class_board.h>
#include <common.h>
#include <convert_basic_shapes_to_polygon.h>
#include <gr_text.h>
#include <fctsys.h>
#include <geometry/geometry_utils.h>
#include <gr_basic.h>
#include <math/util.h>      // for KiROUND
#include <layers_id_colors_and_visibility.h>
#include <settings/color_settings.h>
#include <pcb_edit_frame.h>
#include <pcb_screen.h>
#include <pcbnew.h>
#include <trigo.h>


// Helper class to store parameters used to draw a pad
PAD_DRAWINFO::PAD_DRAWINFO()
{
    m_Color           = BLACK;
    m_HoleColor       = BLACK; // could be DARKGRAY;
    m_NPHoleColor     = YELLOW;
    m_NoNetMarkColor  = BLUE;
    m_PadClearance    = 0;
    m_Display_padnum  = true;
    m_Display_netname = true;
    m_ShowPadFilled   = true;
    m_ShowNCMark      = true;
    m_ShowNotPlatedHole = false;
    m_IsPrinting = false;
}


/**
 * Function BuildSegmentFromOvalShape
 * Has meaning only for OVAL (and ROUND) pads.
 * Build an equivalent segment having the same shape as the OVAL shape,
 * aSegStart and aSegEnd are the ending points of the equivalent segment of the shape
 * aRotation is the asked rotation of the segment (usually m_Orient)
 */
int D_PAD::BuildSegmentFromOvalShape( wxPoint& aSegStart, wxPoint& aSegEnd, double aRotation,
                                      const wxSize& aMargin ) const
{
    int width;

    if( m_Size.y < m_Size.x )     // Build an horizontal equiv segment
    {
        int delta   = ( m_Size.x - m_Size.y ) / 2;
        aSegStart.x = -delta - aMargin.x;
        aSegStart.y = 0;
        aSegEnd.x = delta + aMargin.x;
        aSegEnd.y = 0;
        width = m_Size.y + ( aMargin.y * 2 );
    }
    else        // Vertical oval: build a vertical equiv segment
    {
        int delta   = ( m_Size.y -m_Size.x ) / 2;
        aSegStart.x = 0;
        aSegStart.y = -delta - aMargin.y;
        aSegEnd.x = 0;
        aSegEnd.y = delta + aMargin.y;
        width = m_Size.x + ( aMargin.x * 2 );
    }

    if( aRotation )
    {
        RotatePoint( &aSegStart, aRotation);
        RotatePoint( &aSegEnd, aRotation);
    }

    return width;
}


void D_PAD::BuildPadPolygon( wxPoint aCoord[4], wxSize aInflateValue,
                             double aRotation ) const
{
    wxSize delta;
    wxSize halfsize;

    halfsize.x = m_Size.x >> 1;
    halfsize.y = m_Size.y >> 1;

    switch( GetShape() )
    {
        case PAD_SHAPE_RECT:
            // For rectangular shapes, inflate is easy
            halfsize += aInflateValue;

            // Verify if do not deflate more than than size
            // Only possible for inflate negative values.
            if( halfsize.x < 0 )
                halfsize.x = 0;

            if( halfsize.y < 0 )
                halfsize.y = 0;
            break;

        case PAD_SHAPE_TRAPEZOID:
            // Trapezoidal pad: verify delta values
            delta.x = ( m_DeltaSize.x >> 1 );
            delta.y = ( m_DeltaSize.y >> 1 );

            // be sure delta values are not to large
            if( (delta.x < 0) && (delta.x <= -halfsize.y) )
                delta.x = -halfsize.y + 1;

            if( (delta.x > 0) && (delta.x >= halfsize.y) )
                delta.x = halfsize.y - 1;

            if( (delta.y < 0) && (delta.y <= -halfsize.x) )
                delta.y = -halfsize.x + 1;

            if( (delta.y > 0) && (delta.y >= halfsize.x) )
                delta.y = halfsize.x - 1;
        break;

        default:    // is used only for rect and trap. pads
            return;
    }

    // Build the basic rectangular or trapezoid shape
    // delta is null for rectangular shapes
    aCoord[0].x = -halfsize.x - delta.y;     // lower left
    aCoord[0].y = +halfsize.y + delta.x;

    aCoord[1].x = -halfsize.x + delta.y;     // upper left
    aCoord[1].y = -halfsize.y - delta.x;

    aCoord[2].x = +halfsize.x - delta.y;     // upper right
    aCoord[2].y = -halfsize.y + delta.x;

    aCoord[3].x = +halfsize.x + delta.y;     // lower right
    aCoord[3].y = +halfsize.y - delta.x;

    // Offsetting the trapezoid shape id needed
    // It is assumed delta.x or/and delta.y == 0
    if( GetShape() == PAD_SHAPE_TRAPEZOID && (aInflateValue.x != 0 || aInflateValue.y != 0) )
    {
        double angle;
        wxSize corr;

        if( delta.y )    // lower and upper segment is horizontal
        {
            // Calculate angle of left (or right) segment with vertical axis
            angle = atan2( (double) m_DeltaSize.y, (double) m_Size.y );

            // left and right sides are moved by aInflateValue.x in their perpendicular direction
            // We must calculate the corresponding displacement on the horizontal axis
            // that is delta.x +- corr.x depending on the corner
            corr.x  = KiROUND( tan( angle ) * aInflateValue.x );
            delta.x = KiROUND( aInflateValue.x / cos( angle ) );

            // Horizontal sides are moved up and down by aInflateValue.y
            delta.y = aInflateValue.y;

            // corr.y = 0 by the constructor
        }
        else if( delta.x )          // left and right segment is vertical
        {
            // Calculate angle of lower (or upper) segment with horizontal axis
            angle = atan2( (double) m_DeltaSize.x, (double) m_Size.x );

            // lower and upper sides are moved by aInflateValue.x in their perpendicular direction
            // We must calculate the corresponding displacement on the vertical axis
            // that is delta.y +- corr.y depending on the corner
            corr.y  = KiROUND( tan( angle ) * aInflateValue.y );
            delta.y = KiROUND( aInflateValue.y / cos( angle ) );

            // Vertical sides are moved left and right by aInflateValue.x
            delta.x = aInflateValue.x;

            // corr.x = 0 by the constructor
        }
        else                                    // the trapezoid is a rectangle
        {
            delta = aInflateValue;              // this pad is rectangular (delta null).
        }

        aCoord[0].x += -delta.x - corr.x;       // lower left
        aCoord[0].y += delta.y + corr.y;

        aCoord[1].x += -delta.x + corr.x;     // upper left
        aCoord[1].y += -delta.y - corr.y;

        aCoord[2].x += delta.x - corr.x;     // upper right
        aCoord[2].y += -delta.y + corr.y;

        aCoord[3].x += delta.x + corr.x;     // lower right
        aCoord[3].y += delta.y - corr.y;

        /* test coordinates and clamp them if the offset correction is too large:
         * Note: if a coordinate is bad, the other "symmetric" coordinate is bad
         * So when a bad coordinate is found, the 2 symmetric coordinates
         * are set to the minimun value (0)
         */

        if( aCoord[0].x > 0 )       // lower left x coordinate must be <= 0
            aCoord[0].x = aCoord[3].x = 0;

        if( aCoord[1].x > 0 )       // upper left x coordinate must be <= 0
            aCoord[1].x = aCoord[2].x = 0;

        if( aCoord[0].y < 0 )       // lower left y coordinate must be >= 0
            aCoord[0].y = aCoord[1].y = 0;

        if( aCoord[3].y < 0 )       // lower right y coordinate must be >= 0
            aCoord[3].y = aCoord[2].y = 0;
    }

    if( aRotation )
    {
        for( int ii = 0; ii < 4; ii++ )
            RotatePoint( &aCoord[ii], aRotation );
    }
}
