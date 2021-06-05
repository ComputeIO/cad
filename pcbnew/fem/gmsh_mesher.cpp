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

    int maxError = m_board->GetDesignSettings().m_MaxError;

    std::vector<std::pair<int, int>> fragments;
    GMSH_MESHER_REGIONS              regions;

    int                currentHeight = 0;
    int                lastHeight = 0;
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
        double lastHeight_mm = lastHeight / IU_PER_MM;
        double holeHeight_mm = currentHeight_mm - lastHeight_mm;

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
            regions.m_drills.emplace( idx );
        }

        std::set<const PAD*> ignoredPads;
        for( const auto& pad_region : m_pad_regions )
        {
            ignoredPads.emplace( pad_region.second );
            for( const auto& idx :
                 PadTo2DPlaneSurfaces( layer, -currentHeight_mm, pad_region.second, maxError ) )
            {
                fragments.emplace_back( 2, idx );
                regions.m_pads.emplace( idx, pad_region.first );
            }

            if( holeHeight_mm != 0. && pad_region.second->IsOnLayer( layer )
                && pad_region.second->GetAttribute() != PAD_ATTRIB::SMD )
            {
                int padHoleSurface = PadHoleToCurveLoop( pad_region.second, -currentHeight_mm );
                int extrudedSurface = CurveLoopToPlaneSurfaces( padHoleSurface, holeHeight_mm );
                fragments.emplace_back( 2, extrudedSurface );
                regions.m_pads.emplace( extrudedSurface, pad_region.first );
            }
        }

        for( const auto& net_region : m_net_regions )
        {
            std::pair<std::vector<int>, std::vector<int>> netSurfaces =
                    NetTo2DPlaneSurfaces( layer, -currentHeight_mm, net_region.second, maxError );
            for( const auto& idx : netSurfaces.first )
            {
                fragments.emplace_back( 2, idx );
                regions.m_nets.emplace( idx, net_region.first );
            }
            for( const auto& idx : netSurfaces.second )
            {
                fragments.emplace_back( 2, idx );
                regions.m_net_holes.emplace( idx );
            }

            if( holeHeight_mm != 0. )
            {
                // Via holes
                for( const TRACK* track : m_board->Tracks() )
                {
                    if( !track->IsOnLayer( layer ) || !VIA::ClassOf( track ) )
                        continue;

                    int viaHoleSurface = ViaHoleToCurveLoop( static_cast<const VIA*>( track ),
                                                             -currentHeight_mm );
                    int extrudedSurface = CurveLoopToPlaneSurfaces( viaHoleSurface, holeHeight_mm );
                    fragments.emplace_back( 2, extrudedSurface );
                    regions.m_nets.emplace( extrudedSurface, net_region.first );
                }

                // Pad holes
                for( const FOOTPRINT* footprint : m_board->Footprints() )
                {
                    for( const PAD* pad : footprint->Pads() )
                    {
                        if( !pad->IsOnLayer( layer ) || pad->GetAttribute() == PAD_ATTRIB::SMD
                            || ignoredPads.count( pad ) != 0 )
                            continue;

                        int padHoleSurface = PadHoleToCurveLoop( pad, -currentHeight_mm );
                        int extrudedSurface =
                                CurveLoopToPlaneSurfaces( padHoleSurface, holeHeight_mm );
                        fragments.emplace_back( 2, extrudedSurface );
                        regions.m_nets.emplace( extrudedSurface, net_region.first );
                    }
                }
            }
        }

        lastHeight = currentHeight;
    }

    std::cerr << "fragment:" << std::endl;
    std::vector<std::pair<int, int>>              ov;
    std::vector<std::vector<std::pair<int, int>>> ovv;
    gmsh::model::occ::fragment( fragments, {}, ov, ovv );

    std::map<int, std::vector<int>> regionToShapeId =
            RegionsToShapesAfterFragment( fragments, ov, ovv, regions );

    int airTag = m_next_region_id; // TODO

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


void GMSH_MESHER::Load3DMesh()
{
    gmsh::initialize();

    // (1: MeshAdapt, 2: Automatic, 3: Initial mesh only, 5: Delaunay, 6: Frontal-Delaunay, 7: BAMG, 8: Frontal-Delaunay for Quads, 9: Packing of Parallelograms)
    gmsh::option::setNumber( "Mesh.Algorithm", 2 );

    gmsh::model::add( "pcb" );

    int maxError = m_board->GetDesignSettings().m_MaxError;

    std::vector<std::pair<int, int>> fragments;
    GMSH_MESHER_REGIONS              regions;

    int                currentHeight = 0;
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
        double currentThickness_mm = item->GetThickness( 0 ) / IU_PER_MM;

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
        std::cout << "render layer " << layer << " with offset: " << currentHeight_mm
                  << " - Thickness: " << currentThickness_mm << std::endl;

        for( const auto& idx : PlaneSurfacesToVolumes(
                     HolesTo2DPlaneSurfaces( layer, -currentHeight_mm ), currentThickness_mm ) )
        {
            fragments.emplace_back( 3, idx );
            regions.m_drills.emplace( idx );
        }

        for( const auto& pad_region : m_pad_regions )
        {
            for( const auto& idx :
                 PlaneSurfacesToVolumes( PadTo2DPlaneSurfaces( layer, -currentHeight_mm,
                                                               pad_region.second, maxError ),
                                         currentThickness_mm ) )
            {
                fragments.emplace_back( 3, idx );
                regions.m_pads.emplace( idx, pad_region.first );
            }
        }

        for( const auto& net_region : m_net_regions )
        {
            std::pair<std::vector<int>, std::vector<int>> netSurfaces =
                    NetTo2DPlaneSurfaces( layer, -currentHeight_mm, net_region.second, maxError );
            for( const auto& idx :
                 PlaneSurfacesToVolumes( netSurfaces.first, currentThickness_mm ) )
            {
                fragments.emplace_back( 3, idx );
                regions.m_nets.emplace( idx, net_region.first );
            }
            for( const auto& idx :
                 PlaneSurfacesToVolumes( netSurfaces.second, currentThickness_mm ) )
            {
                fragments.emplace_back( 3, idx );
                regions.m_net_holes.emplace( idx );
            }
        }

        currentHeight += item->GetThickness( 0 );
    }

    std::cerr << "fragment:" << std::endl;
    std::vector<std::pair<int, int>>              ov;
    std::vector<std::vector<std::pair<int, int>>> ovv;
    gmsh::model::occ::fragment( fragments, {}, ov, ovv );
    std::cerr << "RegionsToShapesAfterFragment:" << std::endl;

    std::map<int, std::vector<int>> regionToShapeId =
            RegionsToShapesAfterFragment( fragments, ov, ovv, regions );

    int airTag = m_next_region_id; // TODO

    // remove air regions
    const auto& airShapes = regionToShapeId.find( airTag );
    if( airShapes != regionToShapeId.end() )
    {
        for( const auto& e : airShapes->second )
        {
            gmsh::model::occ::remove( { { 3, e } }, true );
        }
        regionToShapeId.erase( airShapes->first );
    }

    // we need to do synchronize when calling any non occ function!
    std::cerr << "synchronize(): ";
    gmsh::model::occ::synchronize();
    std::cerr << "FINISHED" << std::endl;

    for( const auto& kv : regionToShapeId )
    {
        gmsh::model::addPhysicalGroup( 3, kv.second, kv.first );
    }

    // generated 3d-mesh
    std::cerr << "generate mesh" << std::endl;
    gmsh::model::mesh::generate( 3 );
    //std::cerr << "refine mesh" << std::endl;
    //gmsh::model::mesh::refine();
    //std::cerr << "set order mesh" << std::endl;
    //gmsh::model::mesh::setOrder(2);
    std::cerr << "finish mesh" << std::endl;

    m_region_surfaces.clear();
}


void GMSH_MESHER::Load3DMeshNew()
{
    gmsh::initialize();

    // (1: MeshAdapt, 2: Automatic, 3: Initial mesh only, 5: Delaunay, 6: Frontal-Delaunay, 7: BAMG, 8: Frontal-Delaunay for Quads, 9: Packing of Parallelograms)
    gmsh::option::setNumber( "Mesh.Algorithm", 2 );

    gmsh::model::add( "pcb" );

    // Generate stackup lookup structure
    GMSH_MESHER_STACKUP stackup;
    stackup.m_copper_plating_thickness = 35000; // nm
    GenerateStackupLookupTable( stackup, false );

    int maxError = m_board->GetDesignSettings().m_MaxError;

    std::vector<std::pair<int, int>> fragments;
    GMSH_MESHER_REGIONS              regions;

    // Pads
    std::set<const PAD*> createdPads;
    for( const auto& pad_region : m_pad_regions )
    {
        const PAD* pad = pad_region.second;
        GeneratePad3D( pad_region.first, pad, stackup, maxError, fragments, regions );
        createdPads.emplace( pad );
    }

    // Net Regions
    for( const auto& net_region : m_net_regions )
    {
        GenerateNet3D( net_region.first, net_region.second, createdPads, stackup, maxError,
                       fragments, regions );
    }

    std::cerr << "fragment:" << std::endl;
    std::vector<std::pair<int, int>>              ov;
    std::vector<std::vector<std::pair<int, int>>> ovv;
    gmsh::model::occ::fragment( fragments, {}, ov, ovv );
    std::cerr << "RegionsToShapesAfterFragment:" << std::endl;

    std::map<int, std::vector<int>> regionToShapeId =
            RegionsToShapesAfterFragment( fragments, ov, ovv, regions );

    int airTag = m_next_region_id; // TODO

    // remove air regions
    const auto& airShapes = regionToShapeId.find( airTag );
    if( airShapes != regionToShapeId.end() )
    {
        for( const auto& e : airShapes->second )
        {
            gmsh::model::occ::remove( { { 3, e } }, true );
        }
        regionToShapeId.erase( airShapes->first );
    }

    // gmsh::model::occ::removeAllDuplicates();
    // gmsh::model::occ::healShapes()

    // we need to do synchronize when calling any non occ function!
    std::cerr << "synchronize(): ";
    gmsh::model::occ::synchronize();
    std::cerr << "FINISHED" << std::endl;

    for( const auto& kv : regionToShapeId )
    {
        gmsh::model::addPhysicalGroup( 3, kv.second, kv.first );
    }

    // generated 3d-mesh
    std::cerr << "generate mesh" << std::endl;
    gmsh::model::mesh::generate( 3 );
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


void GMSH_MESHER::GenerateStackupLookupTable( GMSH_MESHER_STACKUP& stackup,
                                              bool                 copperIsZero ) const
{
    stackup.m_layers.clear();
    stackup.m_dielectric.clear();

    m_board->GetDesignSettings().GetStackupDescriptor().SynchronizeWithBoard(
            &m_board->GetDesignSettings() );
    int currentHeight = 0;
    for( const BOARD_STACKUP_ITEM* item :
         m_board->GetDesignSettings().GetStackupDescriptor().GetList() )
    {
        switch( item->GetType() )
        {
        case BS_ITEM_TYPE_COPPER:
        {
            PCB_LAYER_ID layer = item->GetBrdLayerId();
            double       thickness = item->GetThickness( 0 );

            double startHeight = currentHeight;
            double endHeight = copperIsZero ? currentHeight : currentHeight - thickness;

            stackup.m_layers.emplace( layer, std::pair<double, double>{ startHeight, endHeight } );

            currentHeight = endHeight;

            switch( layer )
            {
            case F_Cu: stackup.m_dielectric.emplace_back( endHeight ); break;
            case B_Cu: stackup.m_dielectric.emplace_back( startHeight ); break;
            default:
                stackup.m_dielectric.emplace_back( ( startHeight + endHeight )
                                                   / 2 ); // dielectric is in the middle
                break;
            }
        }
        break;
        case BS_ITEM_TYPE_DIELECTRIC:
            for( int i = 0; i < item->GetSublayersCount(); i++ )
            {
                currentHeight -= item->GetThickness( i );
                if( i + 1 < item->GetSublayersCount() )
                {
                    stackup.m_dielectric.emplace_back( currentHeight );
                }
            }
            break;
        default: break;
        }
    }

    std::clog << "-- Layer Stackup heights --" << std::endl;
    for( const auto& elem : stackup.m_layers )
    {
        std::clog << " * " << wxString( LSET::Name( elem.first ) ) << " - " << elem.second.first
                  << "nm to " << elem.second.second << "nm" << std::endl;
    }
    std::clog << "-- Layer Dielectric heights --" << std::endl;
    for( const auto& height : stackup.m_dielectric )
    {
        std::clog << " * " << height << "nm" << std::endl;
    }
}


void GMSH_MESHER::GeneratePad3D( int aRegionId, const PAD* aPad,
                                 const GMSH_MESHER_STACKUP& aStackup, int aMaxError,
                                 std::vector<std::pair<int, int>>& aFragments,
                                 GMSH_MESHER_REGIONS&              aRegions )
{
    for( const auto& layers : aStackup.m_layers )
    {
        if( aPad->IsOnLayer( layers.first ) )
        {
            double start_mm = layers.second.first / IU_PER_MM;
            double end_mm = layers.second.second / IU_PER_MM;
            for( const auto& idx : PlaneSurfacesToVolumes(
                         PadTo2DPlaneSurfaces( layers.first, start_mm, aPad, aMaxError ),
                         end_mm - start_mm ) )
            {
                aFragments.emplace_back( 3, idx );
                aRegions.m_pads.emplace( idx, aRegionId );
            }
        }
    }

    // Check if pad has a hole
    if( aPad->GetAttribute() == PAD_ATTRIB::SMD )
        return;

    // TODO: get start/end layer from stackup!
    PCB_LAYER_ID startLayer = F_Cu;
    PCB_LAYER_ID endLayer = B_Cu;

    // TODO: oval holes
    GenerateDrill3D( aRegionId, aStackup, aMaxError, startLayer, endLayer, aPad->GetPosition(),
                     aPad->GetDrillSizeX(), aFragments, aRegions, aRegions.m_pads );
}


void GMSH_MESHER::GenerateNet3D( int aRegionId, int aNetcode,
                                 const std::set<const PAD*>& aIgnoredPads,
                                 const GMSH_MESHER_STACKUP& aStackup, int aMaxError,
                                 std::vector<std::pair<int, int>>& aFragments,
                                 GMSH_MESHER_REGIONS&              aRegions )
{
    for( const auto& layers : aStackup.m_layers )
    {
        double start_mm = layers.second.first / IU_PER_MM;
        double end_mm = layers.second.second / IU_PER_MM;

        // TODO: error?
        std::pair<std::vector<int>, std::vector<int>> netSurfaces =
                NetTo2DPlaneSurfaces( layers.first, start_mm, aNetcode, aMaxError );
        for( const auto& idx : PlaneSurfacesToVolumes( netSurfaces.first, end_mm - start_mm ) )
        {
            aFragments.emplace_back( 3, idx );
            aRegions.m_nets.emplace( idx, aRegionId );
        }
        for( const auto& idx : PlaneSurfacesToVolumes( netSurfaces.second, end_mm - start_mm ) )
        {
            aFragments.emplace_back( 3, idx );
            aRegions.m_net_holes.emplace( idx );
        }
    }

    // Via holes
    for( const TRACK* track : m_board->Tracks() )
    {
        if( track->GetNetCode() != aNetcode || !VIA::ClassOf( track ) )
            continue;

        const VIA* via = static_cast<const VIA*>( track );

        GenerateDrill3D( aRegionId, aStackup, aMaxError, via->TopLayer(), via->BottomLayer(),
                         via->GetPosition(), via->GetDrillValue(), aFragments, aRegions,
                         aRegions.m_nets );
    }

    // Pad holes
    for( const FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( pad->GetNetCode() != aNetcode || aIgnoredPads.count( pad ) != 0
                || pad->GetAttribute() == PAD_ATTRIB::SMD )
                continue;

            // TODO: get start/end layer from stackup!
            PCB_LAYER_ID startLayer = F_Cu;
            PCB_LAYER_ID endLayer = B_Cu;

            // TODO: oval holes
            GenerateDrill3D( aRegionId, aStackup, aMaxError, startLayer, endLayer,
                             pad->GetPosition(), pad->GetDrillSizeX(), aFragments, aRegions,
                             aRegions.m_nets );
        }
    }
}


void GMSH_MESHER::GenerateDrill3D( int aRegionId, const GMSH_MESHER_STACKUP& aStackup,
                                   int aMaxError, PCB_LAYER_ID aLayerStart, PCB_LAYER_ID aLayerEnd,
                                   wxPoint aPosition, int aDrillSize,
                                   std::vector<std::pair<int, int>>& aFragments,
                                   GMSH_MESHER_REGIONS&              aRegions,
                                   std::map<int, int>&               aRegionMapper )
{
    // TODO: oval holes
    double radius = aDrillSize / 2;

    double start_mm = aStackup.m_layers.at( aLayerStart ).first / IU_PER_MM;
    double end_mm = aStackup.m_layers.at( aLayerEnd ).second / IU_PER_MM;

    SHAPE_POLY_SET copperPolyset;
    TransformCircleToPolygon( copperPolyset, aPosition, radius, aMaxError, ERROR_INSIDE );
    copperPolyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    for( const auto& idx : PlaneSurfacesToVolumes(
                 ShapePolySetToPlaneSurfaces( copperPolyset, start_mm ).first, end_mm - start_mm ) )
    {
        aFragments.emplace_back( 3, idx );
        aRegionMapper.emplace( idx, aRegionId );
    }

    // Drill the copper again :)
    SHAPE_POLY_SET holePolyset;
    TransformCircleToPolygon( holePolyset, aPosition, radius - aStackup.m_copper_plating_thickness,
                              aMaxError, ERROR_INSIDE );
    holePolyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    for( const auto& idx : PlaneSurfacesToVolumes(
                 ShapePolySetToPlaneSurfaces( holePolyset, start_mm ).first, end_mm - start_mm ) )
    {
        aFragments.emplace_back( 3, idx );
        aRegions.m_drills.emplace( idx );
    }
}


std::map<int, std::vector<int>>
GMSH_MESHER::RegionsToShapesAfterFragment( const std::vector<std::pair<int, int>>& fragments,
                                           const std::vector<std::pair<int, int>>& ov,
                                           const std::vector<std::vector<std::pair<int, int>>>& ovv,
                                           const GMSH_MESHER_REGIONS& regions )
{
    std::map<int, std::vector<int>> regionToShapeId;
    std::set<int>                   assignedShapeId;

    int airTag = m_next_region_id; // TODO

    // Priority 1: shapes which denote a drill (are converted to air)
    for( std::size_t i = 0; i < fragments.size(); i++ )
    {
        int origTag = fragments.at( i ).second;
        if( regions.m_drills.count( origTag ) )
        {
            for( const auto& ovTag : ovv.at( i ) )
            {
                if( !assignedShapeId.count( ovTag.second ) )
                {
                    regionToShapeId[airTag].emplace_back( ovTag.second );
                    assignedShapeId.emplace( ovTag.second );
                }
            }
        }
    }

    // Priority 2: shapes which denote a pad
    for( std::size_t i = 0; i < fragments.size(); i++ )
    {
        int origTag = fragments.at( i ).second;

        const auto& padRegionFound = regions.m_pads.find( origTag );
        if( padRegionFound != regions.m_pads.end() )
        {
            for( const auto& ovTag : ovv.at( i ) )
            {
                if( !assignedShapeId.count( ovTag.second ) )
                {
                    regionToShapeId[padRegionFound->second].emplace_back( ovTag.second );
                    assignedShapeId.emplace( ovTag.second );
                }
            }
        }
    }

    // Priority 3: shapes which denote holes in net regions (because fracture does not like holes)
    for( std::size_t i = 0; i < fragments.size(); i++ )
    {
        int origTag = fragments.at( i ).second;
        if( regions.m_net_holes.count( origTag ) )
        {
            for( const auto& ovTag : ovv.at( i ) )
            {
                if( !assignedShapeId.count( ovTag.second ) )
                {
                    regionToShapeId[airTag].emplace_back( ovTag.second );
                    assignedShapeId.emplace( ovTag.second );
                }
            }
        }
    }

    // Priority 4: shapes which denote a net region
    for( std::size_t i = 0; i < fragments.size(); i++ )
    {
        int         origTag = fragments.at( i ).second;
        const auto& netRegionFound = regions.m_nets.find( origTag );
        if( netRegionFound != regions.m_nets.end() )
        {
            for( const auto& ovTag : ovv.at( i ) )
            {
                if( !assignedShapeId.count( ovTag.second ) )
                {
                    regionToShapeId[netRegionFound->second].emplace_back( ovTag.second );
                    assignedShapeId.emplace( ovTag.second );
                }
            }
        }
    }

    // TODO: Priority 5: shapes which denote dielectrics
    // TODO: Priority 6: shapes which denote air

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

    return regionToShapeId;
}

int GMSH_MESHER::CurveLoopToPlaneSurfaces( const int aCurveLoop, double aExtrudeZ )
{
    std::vector<std::pair<int, int>> dimTags = { { 1, aCurveLoop } };

    std::vector<std::pair<int, int>> ov;
    gmsh::model::occ::extrude( dimTags, 0, 0, aExtrudeZ, ov );

    for( const auto& idx : ov )
    {
        if( idx.first == 2 )
        {
            return idx.second;
        }
    }
    return -1; // TODO: should never happen
}

std::vector<int> GMSH_MESHER::PlaneSurfacesToVolumes( const std::vector<int> aSurfaces,
                                                      double                 aExtrudeZ )
{
    std::vector<std::pair<int, int>> dimTags;
    dimTags.reserve( aSurfaces.size() );
    for( const int idx : aSurfaces )
    {
        dimTags.emplace_back( 2, idx );
    }

    std::vector<std::pair<int, int>> ov;
    gmsh::model::occ::extrude( dimTags, 0, 0, aExtrudeZ, ov );

    std::vector<int> ret;
    for( const auto& idx : ov )
    {
        if( idx.first == 3 )
        {
            ret.emplace_back( idx.second );
        }
    }
    return ret;
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

        double radius = static_cast<const VIA*>( track )->GetDrillValue() / 2.;
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

int GMSH_MESHER::ViaHoleToCurveLoop( const VIA* aVia, double aOffsetZ, double aCopperOffset )
{
    double radius = aVia->GetDrillValue() / 2. - aCopperOffset;
    return gmsh::model::occ::addCircle( TransformPoint( aVia->GetPosition().x ),
                                        TransformPoint( aVia->GetPosition().y ), aOffsetZ,
                                        TransformPoint( radius ) );
}

int GMSH_MESHER::PadHoleToCurveLoop( const PAD* aPad, double aOffsetZ, double aCopperOffset )
{
    double radiusX = aPad->GetDrillSizeX() / 2. - aCopperOffset;
    double radiusY = aPad->GetDrillSizeY() / 2. - aCopperOffset;

    return gmsh::model::occ::addEllipse( TransformPoint( aPad->GetPosition().x ),
                                         TransformPoint( aPad->GetPosition().y ), aOffsetZ,
                                         TransformPoint( radiusX ), TransformPoint( radiusY ) );
    // TODO: rotate
}

std::vector<int> GMSH_MESHER::PadTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ,
                                                    const PAD* aPad, int aMaxError )
{
    if( !aPad->IsOnLayer( aLayer ) )
        return {};

    SHAPE_POLY_SET polyset;
    TransformPadWithClearanceToPolygon( polyset, aPad, aLayer, 0, aMaxError, ERROR_INSIDE );
    polyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    return ShapePolySetToPlaneSurfaces( polyset, aOffsetZ ).first; // TODO
}


std::pair<std::vector<int>, std::vector<int>>
GMSH_MESHER::NetTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ, const int aNetcode,
                                   int aMaxError )
{
    SHAPE_POLY_SET polyset;

    // including those outlines could lead to small meshing issues (as we need to subtract the shape again)
    std::set<const PAD*> ignoredPads;
    for( const auto& pad : m_pad_regions )
    {
        ignoredPads.emplace( pad.second );
    }

    // inspired by board_items_to_polygon_shape_transform.cpp
    // convert tracks and vias:
    for( const TRACK* track : m_board->Tracks() )
    {
        if( !track->IsOnLayer( aLayer ) || track->GetNetCode() != aNetcode )
            continue;

        track->TransformShapeWithClearanceToPolygon( polyset, aLayer, 0, aMaxError, ERROR_INSIDE );
    }

    // convert pads and other copper items in footprints
    for( const FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( !pad->IsOnLayer( aLayer ) || pad->GetNetCode() != aNetcode
                || ignoredPads.count( pad ) > 0 )
                continue;

            TransformPadWithClearanceToPolygon( polyset, pad, aLayer, 0, aMaxError, ERROR_INSIDE );
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
