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

#include <gal/opengl/opengl_freetype.h>
#include <wx/bitmap.h>
#include <bitmap_base.h>

using namespace KIGFX;

static inline int next_power_of_2( int a )
{
    int r = 2;
    while( r < a )
        r = r << 1;
    return r;
}


OPENGL_FREETYPE::OPENGL_FREETYPE( OPENGL_GAL* aGal ) : m_gal( aGal )
{
}


FT_BitmapGlyph OPENGL_FREETYPE::getBitmapForGlyph( FT_Glyph& aGlyph ) const
{
    // Render bitmap
    FT_Glyph_To_Bitmap( &aGlyph, FT_RENDER_MODE_NORMAL, 0, 1 );

    return (FT_BitmapGlyph) aGlyph;
}


void OPENGL_FREETYPE::Render( FT_Glyph& aGlyph, const wxPoint& aPosition )
{
#if 0
    FT_BitmapGlyph bitmapGlyph = getBitmapForGlyph( aGlyph );
    FT_Bitmap*     bitmap = &( bitmapGlyph->bitmap );

    int depth = 1;

    switch( bitmap->pixel_mode )
    {
    case FT_PIXEL_MODE_GRAY:
    case FT_PIXEL_MODE_LCD:
    case FT_PIXEL_MODE_LCD_V:
        depth = 8;

        default: depth = 1;
    }


    wxBitmap* wb = new wxBitmap( bitmap->buffer, bitmap->width, bitmap->rows, depth );
    BITMAP_BASE bm( aPosition );
    bm.SetBitmap( wb );
    m_gal.DrawBitmap( bm );


    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStore(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RG8, aPosition.x, aPosition.y, GL_RG, GL_UNSIGNED_BYTE,
                  buffer.data() );
    glPixelStore( GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif

    FT_Done_Glyph( aGlyph );
}
