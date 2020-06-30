/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <preview_items/two_point_assistant.h>

#include <preview_items/draw_context.h>
#include <preview_items/preview_utils.h>

#include <view/view.h>

#include <common.h>
#include <pcb_painter.h>

using namespace KIGFX::PREVIEW;

TWO_POINT_ASSISTANT::TWO_POINT_ASSISTANT(
        const TWO_POINT_GEOMETRY_MANAGER& aManager, EDA_UNITS aUnits, GEOM_SHAPE aShape )
        : EDA_ITEM( NOT_USED ), m_constructMan( aManager ), m_units( aUnits ), m_shape( aShape )
{
}


const BOX2I TWO_POINT_ASSISTANT::ViewBBox() const
{
    BOX2I tmp;

    // no bounding box when no graphic shown
    if( m_constructMan.IsReset() )
        return tmp;

    // just enclose the whle circular area
    auto origin = m_constructMan.GetOrigin();
    auto end    = m_constructMan.GetEnd();

    if( m_shape == GEOM_SHAPE::SEGMENT || m_shape == GEOM_SHAPE::RECT )
    {
        tmp.SetOrigin( origin );
        tmp.SetEnd( end );
    }
    else
    {
        tmp.SetOrigin( origin + end );
        tmp.SetEnd( origin - end );
    }

    tmp.Normalize();
    return tmp;
}


void TWO_POINT_ASSISTANT::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    auto& gal = *aView->GetGAL();

    // not in a position to draw anything
    if( m_constructMan.IsReset() )
        return;

    gal.SetLineWidth( 1.0 );
    gal.SetIsStroke( true );
    gal.SetIsFill( true );

    gal.ResetTextAttributes();

    // constant text size on screen
    SetConstantGlyphHeight( gal, 12.0 );

    const auto origin = m_constructMan.GetOrigin();
    const auto end    = m_constructMan.GetEnd();

    std::vector<wxString> cursorStrings;

    const auto radVec = end - origin;

    if( m_shape == GEOM_SHAPE::SEGMENT )
    {
        if( m_constructMan.GetAngleSnap() )
        {
            const auto absX = std::abs( radVec.x );
            const auto absY = std::abs( radVec.y );

            const double minDistance = std::min( absX, absY );
            const double maxDistance = std::max( absX, absY );

            const double lin45Distance = VECTOR2D( minDistance, minDistance ).EuclideanNorm();

            const double distance = std::max( lin45Distance, maxDistance - minDistance );
            cursorStrings.push_back( DimensionLabel( "d", distance, m_units ) );
        }
        else
        {
            cursorStrings.push_back( DimensionLabel( "d", radVec.EuclideanNorm(), m_units ) );
        }
    }
    else if( m_shape == GEOM_SHAPE::RECT )
    {
        cursorStrings.push_back( DimensionLabel( "x", std::abs( radVec.x ), m_units ) );
        cursorStrings.push_back( DimensionLabel( "y", std::abs( radVec.y ), m_units ) );
    }
    else
    {
        KIGFX::PREVIEW::DRAW_CONTEXT preview_ctx( *aView );
        preview_ctx.DrawLine( origin, end, false );

        cursorStrings.push_back( DimensionLabel( "r", radVec.EuclideanNorm(), m_units ) );
    }

    // place the text next to cursor, on opposite side from drawing
    DrawTextNextToCursor( aView, end, origin - end, cursorStrings );
}