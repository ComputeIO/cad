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

#ifndef OPENGL_FREETYPE_H
#define OPENGL_FREETYPE_H

#include <vector>
#include <utf8.h>
#include <GL/gl.h>
#include <freetype2/ft2build.h>
#include FT_GLYPH_H
#include <wx/gdicmn.h>

class OUTLINE_FONT;

namespace KIGFX
{
class OPENGL_GAL;

class OPENGL_FREETYPE
{
public:
    OPENGL_FREETYPE( OPENGL_GAL* aGal );

    void Render( FT_Glyph& aGlyph, const wxPoint& aPosition );

private:
    OPENGL_GAL* m_gal;

    FT_BitmapGlyph getBitmapForGlyph( FT_Glyph& aGlyph ) const;
};

} // namespace KIGFX

#endif // OPENGL_FREETYPE_H
