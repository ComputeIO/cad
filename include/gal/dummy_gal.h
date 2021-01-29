/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
 * Copyright (C) 2016-2021 Kicad Developers, see change_log.txt for contributors.
 *
 * Outline font class
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

#ifndef DUMMYGAL_H_
#define DUMMYGAL_H_

#include <gal/graphics_abstraction_layer.h>

namespace KIGFX
{
class DUMMY_GAL : public GAL
{
public:
    DUMMY_GAL( GAL_DISPLAY_OPTIONS& aOptions ) : GAL( aOptions ) {}

    /// @copydoc GAL::DrawPolyline()
    void DrawPolyline( const std::vector<VECTOR2D>& aPointList ) override {}

    /// @copydoc GAL::DrawGlyph()
    void DrawGlyph( const SHAPE_POLY_SET& aPolySet, int aNth, int aTotal ) override {}
};
} // namespace KIGFX

#endif // DUMMYGAL_H_
