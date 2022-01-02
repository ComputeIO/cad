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

#include "board_design_settings.h"
#include <convert_basic_shapes_to_polygon.h>

#include "board.h"
#include "footprint.h"
#include "pad.h"
#include "pcb_track.h"
#include "zone.h"
#include "layer_ids.h"

#include <sparselizard/shape.h>


void SPARSELIZARD_MESHER::Get2DShapes( std::vector<shape>& aShapes, PCB_LAYER_ID aLayer,
                                       bool substractHoles )
{
    for( int regionId = 1; regionId < m_next_region_id; regionId++ )
    {
        Get2DShapes( aShapes, regionId, aLayer, substractHoles );
    }
}


void SPARSELIZARD_MESHER::Get3DShapes( std::vector<shape>& aShapes, bool substractHoles )
{
    int currentHeight = 0;

    // TODO: better way?
    m_board->GetDesignSettings().GetStackupDescriptor().SynchronizeWithBoard(
            &m_board->GetDesignSettings() );

    // only through-holes (no blind vias for now)

    std::map<int, std::vector<shape>> holeShapes;
    for( int regionId = 1; regionId < m_next_region_id; regionId++ )
    {
        SHAPE_POLY_SET aHolePolygons;
        SetPolysetOfHolewallOfNetRegion( aHolePolygons, regionId, IU_PER_MM / 100 );

        Triangulate( holeShapes[regionId], aHolePolygons, regionId, 0 );
    }

    for( const BOARD_STACKUP_ITEM* item :
         m_board->GetDesignSettings().GetStackupDescriptor().GetList() )
    {
        if( item->GetType() != BS_ITEM_TYPE_COPPER && item->GetType() != BS_ITEM_TYPE_DIELECTRIC )
        {
            continue;
        }

        int    thickness = item->GetThickness( 0 );
        double thickness_mm = thickness / IU_PER_MM;
        double currentHeight_mm = currentHeight / IU_PER_MM;

        for( auto& elem : holeShapes )
        {
            for( shape& curShape : elem.second )
            {
                shape newShape = curShape.extrude( elem.first, -thickness_mm, 2 );
                newShape.shift( 0, 0, -currentHeight_mm );
                aShapes.emplace_back( newShape );
            }
        }

        std::cout << "item " << item->GetType() << std::endl;
        // We only model copper for now
        if( item->GetType() != BS_ITEM_TYPE_COPPER )
        {
            for( int i = 0; i < item->GetSublayersCount(); i++ )
            {
                currentHeight += item->GetThickness( i );
            }
            continue;
        }

        PCB_LAYER_ID layer = item->GetBrdLayerId();
        std::cout << "render layer " << layer << " with offset: " << std::endl;

        for( int regionId = 1; regionId < m_next_region_id; regionId++ )
        {
            std::vector<shape> aLayerShapes;
            Get2DShapes( aLayerShapes, regionId, layer, substractHoles );

            // move layer upwards // TODO: create in-place
            if( currentHeight != 0 )
            {
                for( auto& shape : aLayerShapes )
                {
                    shape.shift( 0, 0, -currentHeight_mm );
                }
            }

            // create 3d-volume
            for( auto& shape : aLayerShapes )
            {
                aShapes.emplace_back( shape.extrude( regionId, -thickness_mm, 2 ) );
            }
        }

        // TODO: make holes

        currentHeight += thickness;
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

    Triangulate( aShapes, polyset, aRegionId, 0 );
}

void SPARSELIZARD_MESHER::Triangulate( std::vector<shape>& aShapes, SHAPE_POLY_SET& aPolyset,
                                       int aRegionId, double zOffset ) const
{
    aPolyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    // triangulate polygon and generate shape
    // TODO: use triangulation algorithm which is more suited for FEM
    // https://people.sc.fsu.edu/~jburkardt/presentations/mesh_2012_fsu.pdf
    aPolyset.CacheTriangulation();

    std::vector<shape> shapes;
    for( size_t i = 0; i < aPolyset.TriangulatedPolyCount(); i++ )
    {
        const SHAPE_POLY_SET::TRIANGULATED_POLYGON* triangulated =
                aPolyset.TriangulatedPolygon( i );

        for( size_t t = 0; t < triangulated->GetTriangleCount(); t++ )
        {
            VECTOR2<int> a, b, c;
            triangulated->GetTriangle( t, a, b, c );

            std::vector<double> coords = { a.x / IU_PER_MM, a.y / IU_PER_MM, zOffset,
                                           b.x / IU_PER_MM, b.y / IU_PER_MM, zOffset,
                                           c.x / IU_PER_MM, c.y / IU_PER_MM, zOffset };

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
    for( const PCB_TRACK* track : m_board->Tracks() )
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

    for( const PCB_TRACK* track : m_board->Tracks() )
    {
        // assuming proper zone fills and no overlapping vias and pads of different nets!!!
        if( track->GetNetCode() != netcode || !track->IsOnLayer( aLayer )
            || !PCB_VIA::ClassOf( track ) )
            continue;

        int radius = ( static_cast<const PCB_VIA*>( track )->GetDrill() / 2 );
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


void SPARSELIZARD_MESHER::SetPolysetOfHolewallOfNetRegion( SHAPE_POLY_SET& aPolyset, int aRegionId,
                                                           int aThickness ) const
{
    const auto&    netRegion = m_net_regions.find( aRegionId );
    SHAPE_POLY_SET substractPolyset;

    if( netRegion == m_net_regions.end() )
    {
        const auto& padRegion = m_pad_regions.find( aRegionId );
        if( padRegion == m_pad_regions.end() )
        {
            return;
        }

        SetPolysetOfPadDrill( aPolyset, padRegion->second, 0 );
        SetPolysetOfPadDrill( substractPolyset, padRegion->second, aThickness );
    }
    else
    {
        int netcode = netRegion->second;
        int maxError = m_board->GetDesignSettings().m_MaxError;

        for( const PCB_TRACK* track : m_board->Tracks() )
        {
            // assuming proper zone fills and no overlapping vias and pads of different nets!!!
            if( track->GetNetCode() != netcode || !PCB_VIA::ClassOf( track ) )
                continue;

            int radius = ( static_cast<const PCB_VIA*>( track )->GetDrill() / 2 );
            TransformCircleToPolygon( aPolyset, track->GetPosition(), radius, maxError,
                                      ERROR_INSIDE );
            TransformCircleToPolygon( substractPolyset, track->GetPosition(), radius - aThickness,
                                      maxError, ERROR_INSIDE );
        }

        for( const FOOTPRINT* footprint : m_board->Footprints() )
        {
            for( const PAD* pad : footprint->Pads() )
            {
                if( std::find_if( m_pad_regions.begin(), m_pad_regions.end(),
                                  [pad]( const auto& mo ) {
                                      return mo.second == pad;
                                  } )
                    != m_pad_regions.end() )
                {
                    continue; // pad is used in pad region
                }

                if( pad->GetNetCode() != netcode )
                {
                    continue;
                }

                SetPolysetOfPadDrill( aPolyset, pad, 0 );
                SetPolysetOfPadDrill( substractPolyset, pad, aThickness );
            }
        }
    }
    aPolyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );
    substractPolyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    aPolyset.BooleanAdd( substractPolyset, SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    aPolyset.BooleanSubtract( substractPolyset, SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );
}

void SPARSELIZARD_MESHER::SetPolysetOfPadDrill( SHAPE_POLY_SET& aPolyset, const PAD* pad,
                                                int thicknessModifier ) const
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
                                  pad->GetDrillSizeX() / 2 - thicknessModifier, maxError,
                                  ERROR_INSIDE );
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
        clearance.x += pad->GetLocalSolderMaskMargin();
        clearance.y += pad->GetLocalSolderMaskMargin();
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
