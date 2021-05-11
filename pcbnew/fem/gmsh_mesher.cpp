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

#include "gmsh_mesher.h"
#include <gmsh.h>

#include <convert_basic_shapes_to_polygon.h>

#include "board.h"
#include "pad.h"
#include "track.h"

double TransformPoint( double aCoordinate )
{
    return aCoordinate / IU_PER_MM;
}

void GMSH_MESHER::Load25DMesh()
{
    gmsh::initialize();

    // (1: MeshAdapt, 2: Automatic, 3: Initial mesh only, 5: Delaunay, 6: Frontal-Delaunay, 7: BAMG, 8: Frontal-Delaunay for Quads, 9: Packing of Parallelograms)
    gmsh::option::setNumber( "Mesh.Algorithm", 2 );

    gmsh::model::add( "pcb" );

    int currentHeight = 0;
    m_board->GetDesignSettings().GetStackupDescriptor().SynchronizeWithBoard(
            &m_board->GetDesignSettings() );
    for( const BOARD_STACKUP_ITEM* item :
         m_board->GetDesignSettings().GetStackupDescriptor().GetList() )
    {
        if( item->GetType() != BS_ITEM_TYPE_COPPER && item->GetType() != BS_ITEM_TYPE_DIELECTRIC )
        {
            continue;
        }

        double currentHeight_mm = currentHeight / IU_PER_MM;

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
        std::cout << "render layer " << layer << " with offset: " << currentHeight_mm << std::endl;

        for( int regionId = 1; regionId < m_next_region_id; regionId++ )
        {
            const std::vector<int> gmshRegionSurfaces =
                    Mesh2DRegion( layer, -currentHeight_mm, regionId, true );
            for( int surface : gmshRegionSurfaces )
            {
                m_region_surfaces[regionId].push_back( surface );
            }
        }
    }

    // we need to do synchronize when calling any non occ function!
    gmsh::model::occ::synchronize();

    for( auto& regions : m_region_surfaces )
    {
        gmsh::model::addPhysicalGroup( 2, regions.second );
        //gmsh::model::setPhysicalName(2, ps, "My surface");
    }

    // generated 2.5d-mesh
    std::cerr << "generate mesh" << std::endl;
    gmsh::model::mesh::generate( 2 );
    //std::cerr << "refine mesh" << std::endl;
    //gmsh::model::mesh::refine();
    //std::cerr << "set order mesh" << std::endl;
    //gmsh::model::mesh::setOrder(2);
    std::cerr << "finish mesh" << std::endl;

    m_region_surfaces.clear();
}


void GMSH_MESHER::Finalize()
{
    gmsh::finalize();
}


std::vector<int> GMSH_MESHER::Mesh2DZone( PCB_LAYER_ID aLayer, double aOffsetZ, const ZONE* zone )
{
    SHAPE_POLY_SET polyset;
    zone->TransformSolidAreasShapesToPolygon( aLayer, polyset );
    polyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    return MeshShapePolySetToPlaneSurfaces( polyset, aOffsetZ );
}


std::vector<int> GMSH_MESHER::Mesh2DRegion( PCB_LAYER_ID aLayer, double aOffsetZ, int aRegionId,
                                            bool substractHoles )
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
        return {};
    }

    if( !substractPolyset.IsEmpty() )
    {
        polyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );
        substractPolyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );
        polyset.BooleanSubtract( substractPolyset, SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );
    }

    return MeshShapePolySetToPlaneSurfaces( polyset, aOffsetZ );
}


std::vector<int> GMSH_MESHER::MeshShapePolySetToPlaneSurfaces( const SHAPE_POLY_SET& aPolySet,
                                                               double                aOffsetZ )
{
    std::vector<int> gmshPlaneSurfaces;
    gmshPlaneSurfaces.reserve( aPolySet.OutlineCount() );
    for( int outline = 0; outline < aPolySet.OutlineCount(); outline++ )
    {
        std::vector<int> gmshCurveLoops;
        gmshCurveLoops.reserve( aPolySet.HoleCount( outline ) + 1 );

        const SHAPE_LINE_CHAIN& outlinePoints = aPolySet.COutline( outline );
        gmshCurveLoops.emplace_back( MeshShapeLineChainToCurveLoop( outlinePoints, aOffsetZ ) );

        for( int hole = 0; hole < aPolySet.HoleCount( outline ); hole++ )
        {
            const SHAPE_LINE_CHAIN& holePoints = aPolySet.CHole( outline, hole );
            gmshCurveLoops.emplace_back( MeshShapeLineChainToCurveLoop( holePoints, aOffsetZ ) );
        }

        gmshPlaneSurfaces.emplace_back( gmsh::model::occ::addPlaneSurface( gmshCurveLoops ) );
    }

    return gmshPlaneSurfaces;
}

int GMSH_MESHER::MeshShapeLineChainToCurveLoop( const SHAPE_LINE_CHAIN& aLineChain,
                                                double                  aOffsetZ )
{
    // Create gmsh points
    std::vector<int> gmshOutlinePoints;
    gmshOutlinePoints.reserve( aLineChain.GetPointCount() );
    for( auto point : aLineChain.CPoints() )
    {
        gmshOutlinePoints.emplace_back( gmsh::model::occ::addPoint(
                TransformPoint( point.x ), TransformPoint( point.y ), aOffsetZ ) );
    }

    // Create gmsh lines
    std::vector<int> gmshClosedLinePoints;
    gmshClosedLinePoints.reserve( gmshOutlinePoints.size() );
    for( size_t i = 0; i + 1 < gmshOutlinePoints.size(); i++ )
    {
        gmshClosedLinePoints.emplace_back( gmsh::model::occ::addLine(
                gmshOutlinePoints.at( i ), gmshOutlinePoints.at( i + 1 ) ) );
    }
    gmshClosedLinePoints.emplace_back( gmsh::model::occ::addLine(
            gmshOutlinePoints.at( gmshOutlinePoints.size() - 1 ), gmshOutlinePoints.at( 0 ) ) );

    // Create gmsh surface loop
    return gmsh::model::occ::addCurveLoop( gmshClosedLinePoints );
}

void GMSH_MESHER::SetPolysetOfNetRegion( SHAPE_POLY_SET& aPolyset, int aRegionId,
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


void GMSH_MESHER::SetPolysetOfHolesOfNetRegion( SHAPE_POLY_SET& aPolyset, int aRegionId,
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


void GMSH_MESHER::SetPolysetOfHolewallOfNetRegion( SHAPE_POLY_SET& aPolyset, int aRegionId,
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

        for( const TRACK* track : m_board->Tracks() )
        {
            // assuming proper zone fills and no overlapping vias and pads of different nets!!!
            if( track->GetNetCode() != netcode || !VIA::ClassOf( track ) )
                continue;

            int radius = ( static_cast<const VIA*>( track )->GetDrill() / 2 );
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

void GMSH_MESHER::SetPolysetOfPadDrill( SHAPE_POLY_SET& aPolyset, const PAD* pad,
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


void GMSH_MESHER::SetPolysetOfPadRegion( SHAPE_POLY_SET& aPolyset, int aRegionId,
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
void GMSH_MESHER::TransformPadWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer, const PAD* pad,
                                                      PCB_LAYER_ID aLayer, int aClearance,
                                                      int aMaxError, ERROR_LOC aErrorLoc ) const
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