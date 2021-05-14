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

    // TODO: better solution to track which elements belong to which surface
    std::vector<std::pair<int, int>> fragments;

    std::map<int, int> padRegions;
    std::map<int, int> netRegions;
    std::set<int>      padHoleTags;
    std::set<int>      holeTags;
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

        for( const auto& idx : HolesTo2DPlaneSurfaces( layer, -currentHeight_mm ) )
        {
            fragments.emplace_back( 2, idx );
            padHoleTags.emplace( idx );
        }

        for( const auto& pad_region : m_pad_regions )
        {
            for( const auto& idx :
                 PadTo2DPlaneSurfaces( layer, -currentHeight_mm, pad_region.second ) )
            {
                fragments.emplace_back( 2, idx );
                padRegions.emplace( idx, pad_region.first );
            }
        }

        for( const auto& net_region : m_net_regions )
        {
            std::pair<std::vector<int>, std::vector<int>> netSurfaces =
                    NetTo2DPlaneSurfaces( layer, -currentHeight_mm, net_region.second );
            for( const auto& idx : netSurfaces.first )
            {
                fragments.emplace_back( 2, idx );
                netRegions.emplace( idx, net_region.first );
            }
            for( const auto& idx : netSurfaces.second )
            {
                fragments.emplace_back( 2, idx );
                holeTags.emplace( idx );
            }
        }
    }

    std::cerr << "fragment:" << std::endl;
    std::vector<std::pair<int, int>>              ov;
    std::vector<std::vector<std::pair<int, int>>> ovv;
    gmsh::model::occ::fragment( fragments, {}, ov, ovv );

    std::map<int, std::vector<int>> regionToShapeId;
    std::set<int>                   assignedShapeId;

    int airTag = m_next_region_id; // TODO

    // holes
    for( std::size_t i = 0; i < fragments.size(); i++ )
    {
        int origTag = fragments.at( i ).second;
        if( padHoleTags.count( origTag ) )
        {
            for( const auto ovTag : ovv.at( i ) )
            {
                if( !assignedShapeId.count( ovTag.second ) )
                {
                    regionToShapeId[airTag].emplace_back( ovTag.second );
                    assignedShapeId.emplace( ovTag.second );
                }
            }
        }
    }
    // pads
    for( std::size_t i = 0; i < fragments.size(); i++ )
    {
        int origTag = fragments.at( i ).second;

        const auto& padRegionFound = padRegions.find( origTag );
        if( padRegionFound != padRegions.end() )
        {
            for( const auto ovTag : ovv.at( i ) )
            {
                if( !assignedShapeId.count( ovTag.second ) )
                {
                    regionToShapeId[padRegionFound->second].emplace_back( ovTag.second );
                    assignedShapeId.emplace( ovTag.second );
                }
            }
        }
    }
    // region holes
    for( std::size_t i = 0; i < fragments.size(); i++ )
    {
        int origTag = fragments.at( i ).second;
        if( holeTags.count( origTag ) )
        {
            for( const auto ovTag : ovv.at( i ) )
            {
                if( !assignedShapeId.count( ovTag.second ) )
                {
                    regionToShapeId[airTag].emplace_back( ovTag.second );
                    assignedShapeId.emplace( ovTag.second );
                }
            }
        }
    }
    // regions
    for( std::size_t i = 0; i < fragments.size(); i++ )
    {
        int         origTag = fragments.at( i ).second;
        const auto& netRegionFound = netRegions.find( origTag );
        if( netRegionFound != netRegions.end() )
        {
            for( const auto ovTag : ovv.at( i ) )
            {
                if( !assignedShapeId.count( ovTag.second ) )
                {
                    regionToShapeId[netRegionFound->second].emplace_back( ovTag.second );
                    assignedShapeId.emplace( ovTag.second );
                }
            }
        }
    }
    // remove air regions
    const auto& airShapes = regionToShapeId.find( airTag );
    if( airShapes != regionToShapeId.end() )
    {
        for( const auto& e : airShapes->second )
        {
            gmsh::model::occ::remove( { { 2, e } }, true );
        }
        regionToShapeId.erase( airShapes->first );
    }

    if( assignedShapeId.size() != ov.size() )
    {
        std::cerr << "!!!!!!!!!!!!!!!!!!!!! SOMETHING WAS NOT ASSIGNED !!!!!!!!!!!!!!" << std::endl;
        // Assign everything else
        for( std::size_t i = 0; i < ov.size(); i++ )
        {
            int ovTag = ov[i].second;
            if( !assignedShapeId.count( ovTag ) )
                regionToShapeId[airTag + 1].emplace_back( ov[i].second );
        }
    }

    // we need to do synchronize when calling any non occ function!
    std::cerr << "synchronize(): ";
    gmsh::model::occ::synchronize();
    std::cerr << "FINISHED" << std::endl;

    for( const auto& kv : regionToShapeId )
    {
        gmsh::model::addPhysicalGroup( 2, kv.second, kv.first );
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


std::pair<std::vector<int>, std::vector<int>>
GMSH_MESHER::ShapePolySetToPlaneSurfaces( const SHAPE_POLY_SET& aPolySet, double aOffsetZ )
{
    std::vector<int> gmshPlaneSurfaces;
    std::vector<int> gmshHoleSurfaces;
    gmshPlaneSurfaces.reserve( aPolySet.OutlineCount() );
    for( int outline = 0; outline < aPolySet.OutlineCount(); outline++ )
    {
        const SHAPE_LINE_CHAIN& outlinePoints = aPolySet.COutline( outline );
        int                     OutlineId = ShapeLineChainToCurveLoop( outlinePoints, aOffsetZ );
        gmshPlaneSurfaces.emplace_back( gmsh::model::occ::addPlaneSurface( { OutlineId } ) );

        for( int hole = 0; hole < aPolySet.HoleCount( outline ); hole++ )
        {
            const SHAPE_LINE_CHAIN& holePoints = aPolySet.CHole( outline, hole );
            int                     holeId = ShapeLineChainToCurveLoop( holePoints, aOffsetZ );
            gmshHoleSurfaces.emplace_back( gmsh::model::occ::addPlaneSurface( { holeId } ) );
        }
    }

    return { gmshPlaneSurfaces, gmshHoleSurfaces };
}

int GMSH_MESHER::ShapeLineChainToCurveLoop( const SHAPE_LINE_CHAIN& aLineChain, double aOffsetZ )
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
    gmshClosedLinePoints.emplace_back(
            gmsh::model::occ::addLine( gmshOutlinePoints.back(), gmshOutlinePoints.at( 0 ) ) );

    // Create gmsh surface loop
    return gmsh::model::occ::addCurveLoop( gmshClosedLinePoints );
}


std::vector<int> GMSH_MESHER::HolesTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ )
{
    std::vector<int> gmshSurfaces;

    int maxError = m_board->GetDesignSettings().m_MaxError;

    // Vias
    for( const TRACK* track : m_board->Tracks() )
    {
        if( !track->IsOnLayer( aLayer ) || !VIA::ClassOf( track ) )
            continue;

        double radius = static_cast<const VIA*>( track )->GetDrill() / 2.;
        gmshSurfaces.emplace_back( gmsh::model::occ::addDisk(
                TransformPoint( track->GetPosition().x ), TransformPoint( track->GetPosition().y ),
                aOffsetZ, TransformPoint( radius ), TransformPoint( radius ) ) );

        // More accurate but with more points as well
        /*SHAPE_POLY_SET polyset;
        TransformCircleToPolygon( polyset, track->GetPosition(), radius, maxError, ERROR_INSIDE );
        polyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

        for(const auto& e : ShapePolySetToPlaneSurfaces(polyset, aOffsetZ).first) {
            gmshSurfaces.emplace_back(e);
        }*/
    }

    // Pads
    for( const FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( !pad->IsOnLayer( aLayer ) || pad->GetAttribute() == PAD_ATTRIB::SMD )
                continue;

            // TODO: oval hole, offset, rotation
            gmshSurfaces.emplace_back( gmsh::model::occ::addDisk(
                    TransformPoint( pad->GetPosition().x ), TransformPoint( pad->GetPosition().y ),
                    aOffsetZ, TransformPoint( pad->GetDrillSizeX() ) / 2.,
                    TransformPoint( pad->GetDrillSizeY() ) / 2. ) );
        }
    }

    return gmshSurfaces;
}


std::vector<int> GMSH_MESHER::PadTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ,
                                                    const PAD* aPad )
{
    if( !aPad->IsOnLayer( aLayer ) )
        return {};

    int maxError = m_board->GetDesignSettings().m_MaxError;

    SHAPE_POLY_SET polyset;
    TransformPadWithClearanceToPolygon( polyset, aPad, aLayer, 0, maxError, ERROR_INSIDE );
    polyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    return ShapePolySetToPlaneSurfaces( polyset, aOffsetZ ).first; // TODO
}


std::pair<std::vector<int>, std::vector<int>>
GMSH_MESHER::NetTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ, const int aNetcode )
{
    SHAPE_POLY_SET polyset;

    // inspired by board_items_to_polygon_shape_transform.cpp
    int maxError = m_board->GetDesignSettings().m_MaxError;

    // convert tracks and vias:
    for( const TRACK* track : m_board->Tracks() )
    {
        if( !track->IsOnLayer( aLayer ) || track->GetNetCode() != aNetcode )
            continue;

        track->TransformShapeWithClearanceToPolygon( polyset, aLayer, 0, maxError, ERROR_INSIDE );
    }

    // convert pads and other copper items in footprints
    for( const FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( !pad->IsOnLayer( aLayer ) || pad->GetNetCode() != aNetcode )
                continue;

            TransformPadWithClearanceToPolygon( polyset, pad, aLayer, 0, maxError, ERROR_INSIDE );
        }

        // TODO: footprints with drawsegments on copper?
        for( const ZONE* zone : footprint->Zones() )
        {
            if( zone->IsFilled() && zone->IsOnLayer( aLayer ) && zone->GetNetCode() == aNetcode )
                zone->TransformSolidAreasShapesToPolygon( aLayer, polyset );
        }
    }

    for( const ZONE* zone : m_board->Zones() )
    {
        if( zone->IsFilled() && zone->IsOnLayer( aLayer ) && zone->GetNetCode() == aNetcode )
            zone->TransformSolidAreasShapesToPolygon( aLayer, polyset );
    }

    polyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    return ShapePolySetToPlaneSurfaces( polyset, aOffsetZ );
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