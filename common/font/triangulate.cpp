/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <font/triangulate.h>

void Triangulate( const SHAPE_POLY_SET& aPolylist, TRIANGULATE_CALLBACK aCallback,
                  void* aCallbackData )
{
    for( int i = 0; i < aPolylist.OutlineCount(); i++ )
    {
        std::vector<std::vector<VECTOR2I>> polygon;
        std::vector<VECTOR2I>              allPoints;
        std::vector<uint32_t>              indices;
        std::vector<VECTOR2I>              outline;

        for( int j = 0; j < aPolylist.COutline( i ).PointCount(); j++ )
        {
            const VECTOR2I& p = aPolylist.COutline( i ).GetPoint( j );
            outline.push_back( p );
            allPoints.push_back( p );
        }
        polygon.push_back( outline );

        std::vector<VECTOR2I> hole;
        for( int k = 0; k < aPolylist.HoleCount( i ); k++ )
        {
            hole.clear();

            for( int m = 0; m < aPolylist.CHole( i, k ).PointCount(); m++ )
            {
                const VECTOR2I& p = aPolylist.CHole( i, k ).GetPoint( m );
                hole.push_back( p );
                allPoints.push_back( p );
            }
            polygon.push_back( hole );
        }

        indices = mapbox::earcut<uint32_t>( polygon );

        for( long unsigned int n = 0; n < indices.size(); n += 3 )
        {
            /* aCallback is called for each triangle in each polygon
             *
             * Parameters:
             * - polygon index
             * - 1st triangle vertex
             * - 2nd triangle vertex
             * - 3rd triangle vertex
             * - aCallbackData
             */
            aCallback( i, allPoints[indices[n]], allPoints[indices[n + 1]],
                       allPoints[indices[n + 2]], aCallbackData );
        }
    }
}
