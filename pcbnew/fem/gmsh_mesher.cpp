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

#include "board.h"
#include "pad.h"

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

    for( auto& netRegion : m_net_regions )
    {
        int netcode = netRegion.second;
        for( const ZONE* zone : m_board->Zones() )
        {
            if( zone->IsFilled() && zone->GetNetCode() == netcode )
            {
                std::vector<int> gmshPlaneSurfaces = Mesh2DZone( PCB_LAYER_ID::F_Cu, zone );
                for( int surface : gmshPlaneSurfaces )
                {
                    m_region_surfaces[netRegion.first].push_back( surface );
                }
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


std::vector<int> GMSH_MESHER::Mesh2DZone( PCB_LAYER_ID aLayer, const ZONE* zone )
{
    double offset = 0;

    SHAPE_POLY_SET polyset;
    zone->TransformSolidAreasShapesToPolygon( aLayer, polyset );
    polyset.Simplify( SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

    std::vector<int> gmshPlaneSurfaces;
    gmshPlaneSurfaces.reserve( polyset.OutlineCount() );
    for( int outline = 0; outline < polyset.OutlineCount(); outline++ )
    {
        std::vector<int> gmshCurveLoops;
        gmshCurveLoops.reserve( polyset.HoleCount( outline ) + 1 );

        const SHAPE_LINE_CHAIN& outlinePoints = polyset.Outline( outline );
        gmshCurveLoops.emplace_back( MeshShapeLineChainToCurveLoop( outlinePoints, offset ) );

        for( int hole = 0; hole < polyset.HoleCount( outline ); hole++ )
        {
            const SHAPE_LINE_CHAIN& holePoints = polyset.Hole( outline, hole );
            gmshCurveLoops.emplace_back( MeshShapeLineChainToCurveLoop( holePoints, offset ) );
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