/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "sparselizard_mesher.h"

#include <convert_basic_shapes_to_polygon.h>

#include "board.h"
#include "pad.h"
#include "track.h"

#include <sparselizard/shape.h>


void SPARSELIZARD_MESHER::Get2DShapes( std::vector<shape>& aShapes, PCB_LAYER_ID aLayer,
                                       bool substractHoles )
{
    for( int regionId = 1; regionId < m_next_region_id; regionId++ )
    {
        Get2DShapes( aShapes, regionId, aLayer, substractHoles );
    }
}


void SPARSELIZARD_MESHER::Get2DShapes( std::vector<shape>& aShapes, int aRegionId,
                                       PCB_LAYER_ID aLayer, bool substractHoles )
{
    SHAPE_POLY_SET polyset;
    SHAPE_POLY_SET substractPolyset;

    if( m_net_regions.count( aRegionId ) )
    {
        SetPolysetOfNetRegion( polyset, aRegionId, aLayer );

        for( auto& padRegion : m_pad_regions )
        {
            SetPolysetOfPadRegion( substractPolyset, padRegion.first, aLayer );
        }

        if( substractHoles )
        {
            SetPolysetOfHolesOfNetRegion( substractPolyset, aRegionId, aLayer );
        }
    }
    else
    {
        SetPolysetOfPadRegion( polyset, aRegionId, aLayer );

        const auto& padRegion = m_pad_regions.find( aRegionId );
        if( substractHoles && padRegion != m_pad_regions.end() )
        {
            SetPolysetOfPadDrill( substractPolyset, padRegion->second );
        }
    }

    if( polyset.IsEmpty() )
    {
        return;
    }

    if( !substractPolyset.IsEmpty() )
    {
        polyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );
        substractPolyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );
        polyset.BooleanSubtract( substractPolyset, SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );
    }

    polyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    // triangulate polygon and generate shape
    // TODO: use triangulation algorithm which is more suited for FEM
    // https://people.sc.fsu.edu/~jburkardt/presentations/mesh_2012_fsu.pdf
    polyset.CacheTriangulation();

    std::vector<shape> shapes;
    for( size_t i = 0; i < polyset.TriangulatedPolyCount(); i++ )
    {
        const SHAPE_POLY_SET::TRIANGULATED_POLYGON* triangulated = polyset.TriangulatedPolygon( i );

        for( size_t t = 0; t < triangulated->GetTriangleCount(); t++ )
        {
            VECTOR2<int> a, b, c;
            triangulated->GetTriangle( t, a, b, c );

            std::vector<double> coords = { a.x / IU_PER_MM, a.y / IU_PER_MM, 0,
                                           b.x / IU_PER_MM, b.y / IU_PER_MM, 0,
                                           c.x / IU_PER_MM, c.y / IU_PER_MM, 0 };

            shape triangle( "triangle", aRegionId, coords, { 2, 2, 2 } );
            aShapes.push_back( triangle );
        }
    }
}


void SPARSELIZARD_MESHER::SetPolysetOfNetRegion( SHAPE_POLY_SET& aPolyset, int aRegionId,
                                                 PCB_LAYER_ID aLayer ) const
{
    const auto& netRegion = m_net_regions.find( aRegionId );
    if( netRegion == m_net_regions.end() )
    {
        return; // nothing found
    }

    int netcode = netRegion->second;
    int maxError = m_board->GetDesignSettings().m_MaxError;

    // inspired by board_items_to_polygon_shape_transform.cpp

    // convert tracks and vias:
    for( const TRACK* track : m_board->Tracks() )
    {
        if( !track->IsOnLayer( aLayer ) || track->GetNetCode() != netcode )
            continue;

        track->TransformShapeWithClearanceToPolygon( aPolyset, aLayer, 0, maxError, ERROR_INSIDE );
    }

    // convert pads and other copper items in footprints
    for( const FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( !pad->IsOnLayer( aLayer ) || pad->GetNetCode() != netcode )
                continue;

            TransformPadWithClearanceToPolygon( aPolyset, pad, aLayer, 0, maxError, ERROR_INSIDE );
        }

        // TODO: footprints with drawsegments on copper?

        for( const ZONE* zone : footprint->Zones() )
        {
            if( zone->IsFilled() && zone->IsOnLayer( aLayer ) )
                zone->TransformSolidAreasShapesToPolygon( aLayer, aPolyset );
        }
    }

    // convert copper zones
    for( const ZONE* zone : m_board->Zones() )
    {
        if( zone->IsFilled() && zone->IsOnLayer( aLayer ) && zone->GetNetCode() == netcode )
            zone->TransformSolidAreasShapesToPolygon( aLayer, aPolyset );
    }
}


void SPARSELIZARD_MESHER::SetPolysetOfHolesOfNetRegion( SHAPE_POLY_SET& aPolyset, int aRegionId,
                                                        PCB_LAYER_ID aLayer ) const
{
    const auto& netRegion = m_net_regions.find( aRegionId );
    if( netRegion == m_net_regions.end() )
    {
        return; // nothing found
    }

    int netcode = netRegion->second;
    int maxError = m_board->GetDesignSettings().m_MaxError;


    for( const TRACK* track : m_board->Tracks() )
    {
        // assuming proper zone fills and no overlapping vias and pads of different nets!!!
        if( track->GetNetCode() != netcode || !track->IsOnLayer( aLayer )
            || !VIA::ClassOf( track ) )
            continue;

        int radius = ( static_cast<const VIA*>( track )->GetDrill() / 2 );
        TransformCircleToPolygon( aPolyset, track->GetPosition(), radius, maxError, ERROR_INSIDE );
    }

    for( const FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            SetPolysetOfPadDrill( aPolyset, pad );
        }
    }
}


void SPARSELIZARD_MESHER::SetPolysetOfPadDrill( SHAPE_POLY_SET& aPolyset, const PAD* pad ) const
{
    if( pad->GetAttribute() == PAD_ATTRIB::SMD )
        return;

    int maxError = m_board->GetDesignSettings().m_MaxError;

    switch( pad->GetDrillShape() )
    {
    case PAD_DRILL_SHAPE_CIRCLE:
    case PAD_DRILL_SHAPE_OBLONG:
        // TODO: correct position, todo oval + rotation correctly
        TransformCircleToPolygon( aPolyset, pad->GetPosition() + pad->GetOffset(),
                                  pad->GetDrillSizeX() / 2, maxError, ERROR_INSIDE );
        break;
    default: break;
    }
}


void SPARSELIZARD_MESHER::SetPolysetOfPadRegion( SHAPE_POLY_SET& aPolyset, int aRegionId,
                                                 PCB_LAYER_ID aLayer ) const
{
    const auto& padRegion = m_pad_regions.find( aRegionId );
    if( padRegion == m_pad_regions.end() )
    {
        return; // nothing found
    }

    const PAD* pad = padRegion->second;
    int        maxError = m_board->GetDesignSettings().m_MaxError;

    if( pad->IsOnLayer( aLayer ) )
        TransformPadWithClearanceToPolygon( aPolyset, pad, aLayer, 0, maxError, ERROR_INSIDE );
}


// TODO: inspired by board_items_to_polygon_shape_transform.cpp and should be simplified there
void SPARSELIZARD_MESHER::TransformPadWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                              const PAD* pad, PCB_LAYER_ID aLayer,
                                                              int aClearance, int aMaxError,
                                                              ERROR_LOC aErrorLoc ) const
{
    if( aLayer != UNDEFINED_LAYER && !pad->IsOnLayer( aLayer ) )
        return;

    if( !pad->FlashLayer( aLayer ) && IsCopperLayer( aLayer ) )
        return;

    // NPTH pads are not drawn on layers if the shape size and pos is the same
    // as their hole:
    if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
    {
        if( pad->GetDrillSize() == pad->GetSize() && pad->GetOffset() == wxPoint( 0, 0 ) )
        {
            switch( pad->GetShape() )
            {
            case PAD_SHAPE::CIRCLE:
                if( pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                    return;
                break;

            case PAD_SHAPE::OVAL:
                if( pad->GetDrillShape() != PAD_DRILL_SHAPE_CIRCLE )
                    return;
                break;

            default: break;
            }
        }
    }

    const bool isPlated = ( ( aLayer == F_Cu ) && pad->FlashLayer( F_Mask ) )
                          || ( ( aLayer == B_Cu ) && pad->FlashLayer( B_Mask ) );

    if( !isPlated )
        return;

    wxSize clearance( aClearance, aClearance );

    switch( aLayer )
    {
    case F_Mask:
    case B_Mask:
        clearance.x += pad->GetSolderMaskMargin();
        clearance.y += pad->GetSolderMaskMargin();
        break;

    case F_Paste:
    case B_Paste: clearance += pad->GetSolderPasteMargin(); break;

    default: break;
    }

    // Our standard TransformShapeWithClearanceToPolygon() routines can't handle differing
    // x:y clearance values (which get generated when a relative paste margin is used with
    // an oblong pad).  So we apply this huge hack and fake a larger pad to run the transform
    // on.
    // Of course being a hack it falls down when dealing with custom shape pads (where the
    // size is only the size of the anchor), so for those we punt and just use clearance.x.

    if( ( clearance.x < 0 || clearance.x != clearance.y ) && pad->GetShape() != PAD_SHAPE::CUSTOM )
    {
        PAD dummy( *pad );
        dummy.SetSize( pad->GetSize() + clearance + clearance );
        dummy.TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, 0, aMaxError,
                                                    aErrorLoc );
    }
    else
    {
        pad->TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, clearance.x, aMaxError,
                                                   aErrorLoc );
    }
}