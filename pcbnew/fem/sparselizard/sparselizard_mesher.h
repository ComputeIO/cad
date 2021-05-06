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

#ifndef KICAD_SPARSELIZARD_MESHER_H
#define KICAD_SPARSELIZARD_MESHER_H

#include <map>
#include <memory>
#include <vector>

#include <layers_id_colors_and_visibility.h>

// sparselizard
class shape;

class SHAPE_POLY_SET;

class BOARD;
class PAD;
class ZONE;

class SPARSELIZARD_MESHER
{
public:
    SPARSELIZARD_MESHER( const BOARD* aBoard ) : m_next_region_id( 1 ), m_board( aBoard ) {}

    int AddNetRegion( int aNetcode )
    {
        return m_net_regions.emplace( m_next_region_id++, aNetcode ).first->first;
    }

    int AddPadRegion( PAD* aPad )
    {
        return m_pad_regions.emplace( m_next_region_id++, aPad ).first->first;
    }

    void Get2DShapes( std::vector<shape>& aShapes, PCB_LAYER_ID aLayer );

    void Get2DShapes( std::vector<shape>& aShapes, int aRegionId, PCB_LAYER_ID aLayer );

private:
    void SetPolysetOfNetRegion( SHAPE_POLY_SET& aPolyset, int aRegionId,
                                PCB_LAYER_ID aLayer ) const;

    void AddZoneToPolyset( SHAPE_POLY_SET& aPolyset, const ZONE* aZone, PCB_LAYER_ID aLayer ) const;

    // TODO: Track, via, pad, hole

    int m_next_region_id;

    std::map<int, int>  m_net_regions;
    std::map<int, PAD*> m_pad_regions;

    const BOARD* m_board;
};


#endif //KICAD_SPARSELIZARD_MESHER_H
