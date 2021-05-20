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

#ifndef KICAD_GMSH_MESHER_H
#define KICAD_GMSH_MESHER_H

#include <map>
#include <memory>
#include <vector>

#include <layers_id_colors_and_visibility.h>
#include <geometry/geometry_utils.h>

// sparselizard
class shape;

class SHAPE_POLY_SET;
class SHAPE_LINE_CHAIN;

class BOARD;
class TRACK;
class PAD;
class ZONE;


class GMSH_MESHER
{
public:
    GMSH_MESHER( const BOARD* aBoard ) : m_next_region_id( 1 ), m_board( aBoard ) {}

    int AddNetRegion( int aNetcode )
    {
        return m_net_regions.emplace( m_next_region_id++, aNetcode ).first->first;
    }

    int AddPadRegion( const PAD* aPad )
    {
        return m_pad_regions.emplace( m_next_region_id++, aPad ).first->first;
    }

    void Load25DMesh();

    void Finalize(); // TODO: automatically called after Load3DMesh?

private:
    std::map<int, std::vector<int>>
    RegionsToShapesAfterFragment( const std::vector<std::pair<int, int>>&              fragments,
                                  const std::vector<std::pair<int, int>>&              ov,
                                  const std::vector<std::vector<std::pair<int, int>>>& ovv,
                                  const std::map<int, int>&                            padRegions,
                                  const std::map<int, int>&                            netRegions,
                                  const std::set<int>& padHoleTags, const std::set<int>& holeTags );

    /**
     * It seems the gmsh::model::occ::fragment code has problems with holes in nets, so we need this workaround
     * @return first vector are outline surfaces, second vector are hole surfaces
     */
    std::pair<std::vector<int>, std::vector<int>>
    ShapePolySetToPlaneSurfaces( const SHAPE_POLY_SET& aPolySet, double aOffsetZ );

    int ShapeLineChainToCurveLoop( const SHAPE_LINE_CHAIN& aLineChain, double aOffsetZ );

    std::vector<int> HolesTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ );

    std::vector<int> PadTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ, const PAD* aPad );

    std::pair<std::vector<int>, std::vector<int>>
    NetTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ, const int aNetcode );

    void TransformPadWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer, const PAD* pad,
                                             PCB_LAYER_ID aLayer, int aClearance, int aMaxError,
                                             ERROR_LOC aErrorLoc ) const;

    int m_next_region_id;

    std::map<int, int>        m_net_regions;
    std::map<int, const PAD*> m_pad_regions;

    std::map<int, std::vector<int>> m_region_surfaces;

    const BOARD* m_board;
};


#endif //KICAD_GMSH_MESHER_H
