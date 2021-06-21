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
class VIA;
class ZONE;


/**
 * Helper structure to track where shapes belong to so they are correctly assigned after fracture.
 */
struct GMSH_MESHER_REGIONS
{
    // Priority 1: shapes which denote a drill (are converted to air)
    std::set<int> m_drills;

    // Priority 2: shapes which denote a pad
    std::map<int, int> m_pads; // shape -> region-id

    // Priority 3: shapes which denote holes in net regions (because fracture does not like holes)
    std::set<int> m_net_holes;

    // Priority 4: shapes which denote a net region
    std::map<int, int> m_nets; // shape -> region-id

    // Priority 5: shapes which denote dielectrics
    std::map<int, int> m_dielectrics;

    // Priority 6: shapes which denote air
    std::set<int> m_air;

    // Priority 7: shapes which denote domain boundary
    std::set<int> m_boundary;
};

struct GMSH_MESHER_LAYER
{
    int    regionID;
    double permittivity;
    double resistivity;
    double lossTangent;
};

struct GMSH_MESHER_STACKUP
{
    std::map<PCB_LAYER_ID, std::pair<int, int>> m_layers;     // start:stop

    std::vector<int> m_dielectric; // transition position

    int m_copper_plating_thickness;
};


class GMSH_MESHER
{
public:
    GMSH_MESHER( const BOARD* aBoard ) :
            m_next_region_id( 1 ), m_air_region( -1 ), m_boundary_region( -1 ), m_board( aBoard )
    {
    }

    int AddNetRegion( int aNetcode )
    {
        return m_net_regions.emplace( m_next_region_id++, aNetcode ).first->first;
    }

    int AddPadRegion( const PAD* aPad )
    {
        return m_pad_regions.emplace( m_next_region_id++, aPad ).first->first;
    }

    std::vector<GMSH_MESHER_LAYER> AddDielectricRegions();

    int AddAirRegion()
    {
        if( m_air_region == -1 )
        {
            m_air_region = m_next_region_id++;
        }
        return m_air_region;
    }

    int AddDomainBoundaryRegion()
    {
        if( m_boundary_region == -1 )
        {
            m_boundary_region = m_next_region_id++;
        }
        return m_boundary_region;
    }

    void Load25DMesh();

    void Load3DMesh();

    void Finalize(); // TODO: automatically called after Load3DMesh?

private:
    void GenerateStackupLookupTable( GMSH_MESHER_STACKUP& stackup,
                                     bool                 copperIsZero = false ) const;

    void GeneratePad3D( int aRegionId, const PAD* aPad, const GMSH_MESHER_STACKUP& aStackup,
                        int aMaxError, std::vector<std::pair<int, int>>& aFragments,
                        GMSH_MESHER_REGIONS& aRegions );

    void GenerateNet3D( int aRegionId, int aNetcode, const std::set<const PAD*>& aIgnoredPads,
                        const GMSH_MESHER_STACKUP& aStackup, int aMaxError,
                        std::vector<std::pair<int, int>>& aFragments,
                        GMSH_MESHER_REGIONS&              aRegions );

    void GenerateDrill3D( int aRegionId, const GMSH_MESHER_STACKUP& aStackup, int aMaxError,
                          PCB_LAYER_ID aLayerStart, PCB_LAYER_ID aLayerEnd, wxPoint aPosition,
                          int aDrillSize, std::vector<std::pair<int, int>>& aFragments,
                          GMSH_MESHER_REGIONS& aRegions, std::map<int, int>& aRegionMapper );

    void GenerateDielectric3D( int aRegionId, const SHAPE_POLY_SET& aPolyset, int aLayerId,
                               const GMSH_MESHER_STACKUP& aStackup, int aMaxError,
                               std::vector<std::pair<int, int>>& aFragments,
                               GMSH_MESHER_REGIONS&              aRegions );

    void GenerateAir3D( const wxPoint aCenter, const wxSize aSize, const int aHeight,
                        std::vector<std::pair<int, int>>& aFragments,
                        GMSH_MESHER_REGIONS&              aRegions );

    std::map<int, std::vector<int>>
    RegionsToShapesAfterFragment( const std::vector<std::pair<int, int>>&              fragments,
                                  const std::vector<std::pair<int, int>>&              ov,
                                  const std::vector<std::vector<std::pair<int, int>>>& ovv,
                                  const GMSH_MESHER_REGIONS&                           regions );

    int CurveLoopToPlaneSurfaces( const int aCurveLoop, double aExtrudeZ );

    std::vector<int> PlaneSurfacesToVolumes( const std::vector<int> aSurfaces, double aExtrudeZ );

    /**
     * It seems the gmsh::model::occ::fragment code has problems with holes in nets, so we need this workaround
     * @return first vector are outline surfaces, second vector are hole surfaces
     */
    std::pair<std::vector<int>, std::vector<int>>
    ShapePolySetToPlaneSurfaces( const SHAPE_POLY_SET& aPolySet, double aOffsetZ );

    int ShapeLineChainToCurveLoop( const SHAPE_LINE_CHAIN& aLineChain, double aOffsetZ );

    std::vector<int> HolesTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ );

    int ViaHoleToCurveLoop( const VIA* aVia, double aOffsetZ, double aCopperOffset = 0 );

    int PadHoleToCurveLoop( const PAD* aPad, double aOffsetZ, double aCopperOffset = 0 );

    std::vector<int> PadTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ, const PAD* aPad,
                                           int aMaxError );

    std::pair<std::vector<int>, std::vector<int>>
    NetTo2DPlaneSurfaces( PCB_LAYER_ID aLayer, double aOffsetZ, const int aNetcode, int aMaxError );

    void TransformPadWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer, const PAD* pad,
                                             PCB_LAYER_ID aLayer, int aClearance, int aMaxError,
                                             ERROR_LOC aErrorLoc ) const;

    int m_next_region_id;

    std::map<int, int>        m_net_regions;
    std::map<int, const PAD*> m_pad_regions;
    std::vector<int>          m_dielectric_regions;
    int                       m_boundary_region;
    int                       m_air_region;

    const BOARD* m_board;
};


#endif //KICAD_GMSH_MESHER_H
