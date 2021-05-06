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

#include "board.h"

#include <sparselizard/shape.h>


void SPARSELIZARD_MESHER::Get2DShapes( std::vector<shape>& aShapes, PCB_LAYER_ID aLayer )
{
    for( int regionId = 1; regionId < m_next_region_id; regionId++ )
    {
        Get2DShapes( aShapes, regionId, aLayer );
    }
}


void SPARSELIZARD_MESHER::Get2DShapes( std::vector<shape>& aShapes, int aRegionId,
                                       PCB_LAYER_ID aLayer )
{
    SHAPE_POLY_SET polyset;
    SetPolysetOfNetRegion( polyset, aRegionId, aLayer );

    if( polyset.IsEmpty() )
    {
        return;
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
    const auto& netRegions = m_net_regions.find( aRegionId );
    if( netRegions == m_net_regions.end() )
    {
        return; // nothing found
    }

    int netcode = netRegions->second;

    for( size_t i = 0; i < m_board->GetAreaCount(); i++ )
    {
        const ZONE* zone = m_board->GetArea( i );
        if( zone->GetNetCode() == netcode )
        {
            AddZoneToPolyset( aPolyset, zone, aLayer );
        }
    }

    // TODO: tracks, pads,...

    // TODO: substract pad and holes from polyset
}


void SPARSELIZARD_MESHER::AddZoneToPolyset( SHAPE_POLY_SET& aPolyset, const ZONE* aZone,
                                            PCB_LAYER_ID aLayer ) const
{
    if( aZone->IsFilled() && aZone->IsOnLayer( aLayer ) )
    {
        aPolyset.BooleanAdd( aZone->GetFilledPolysList( aLayer ),
                             SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );
        // TODO: handle old zones with linewidth
    }
}