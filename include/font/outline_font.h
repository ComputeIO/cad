/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef OUTLINE_FONT_H_
#define OUTLINE_FONT_H_

#include <gal/graphics_abstraction_layer.h>
#include <geometry/shape_poly_set.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
//#include <gal/opengl/opengl_freetype.h>
#include <harfbuzz/hb.h>
#include <font/font.h>
#include <font/outline_decomposer.h>
#include <parser/markup_parser.h>

namespace KIFONT
{
/**
 * Class OUTLINE_FONT implements outline font drawing.
 */
class OUTLINE_FONT : public FONT
{
public:
    OUTLINE_FONT();

    bool IsOutline() const override { return true; }

    bool IsBold() const override { return mFace && ( mFace->style_flags & FT_STYLE_FLAG_BOLD ); }

    bool IsItalic() const override
    {
        return mFace && ( mFace->style_flags & FT_STYLE_FLAG_ITALIC );
    }

    /**
     * Load an outline font. TrueType (.ttf) and OpenType (.otf)
     * are supported.
     * @param aFontFileName is the name of the font file
     * @param aBold is true if font should be bold, otherwise false
     * @param aItalic is true if font should be italic, otherwise false
     * @return true if the font was successfully loaded, otherwise false.
     */
    bool LoadFont( const wxString& aFontFileName, bool aBold, bool aItalic ) override;

#if 0
    /**
     * Draw a string.
     *
     * @param aGal
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aOrigin is the item origin
     * @param aAttributes contains text attributes (angle, line spacing, ...)
     * @return bounding box width/height
     */
    VECTOR2D Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                   const VECTOR2D& aOrigin, const TEXT_ATTRIBUTES& aAttributes ) const override;
#endif

    /**
     * Compute the boundary limits of aText (the bounding box of all shapes).
     *
     * The overbar and alignment are not taken in account, '~' characters are skipped.
     *
     * @return a VECTOR2D giving the width and height of text.
     */
    VECTOR2D StringBoundaryLimits( const KIGFX::GAL* aGal, const UTF8& aText,
                                   const VECTOR2D& aGlyphSize,
                                   double          aGlyphThickness ) const override;

    /**
     * Compute the vertical position of an overbar, sometimes used in texts.
     *
     * This is the distance between the text base line and the overbar.
     *
     * @param aGlyphHeight is the height (vertical size) of the text.
     * @return the relative position of the overbar axis.
     */
    double ComputeOverbarVerticalPosition( double aGlyphHeight ) const override;

    /**
     * Compute the distance (interline) between 2 lines of text (for multiline texts).
     *
     * @param aGlyphHeight is the height (vertical size) of the text.
     * @param aLineSpacing is the line spacing multiplier (defaults to 1.0)
     * @return the interline.
     */
    double GetInterline( double aGlyphHeight = 0.0, double aLineSpacing = 1.0 ) const override;

    /**
     * Compute the X and Y size of a given text. The text is expected to be
     * a only one line text.
     *
     * @param aText is the text string (one line).
     * @return the text size.
     */
    VECTOR2D ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const override;


    VECTOR2I GetTextAsPolygon( std::vector<SHAPE_POLY_SET>& aGlyphs, const UTF8& aText,
                               const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                               const EDA_ANGLE& aAngle, bool aIsMirrored,
                               TEXT_STYLE_FLAGS aTextStyle = 0 ) const;

    /**
     * Like GetTextAsPolygon, but handles multiple lines.
     * TODO: Combine with GetTextAsPolygon, maybe with a boolean parameter,
     * but it's possible a non-line-breaking version isn't even needed
     *
     * @param aGlyphs returns text glyphs
     * @param aText the text item
     */
    VECTOR2I GetLinesAsPolygon( std::vector<SHAPE_POLY_SET>& aGlyphs, const EDA_TEXT* aText ) const;

    const FT_Face& GetFace() const { return mFace; }

#if 0
    void RenderToOpenGLCanvas( KIGFX::OPENGL_FREETYPE& aTarget, const UTF8& aString,
                               const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                               double aOrientation, bool aIsMirrored ) const;
#endif

protected:
    VECTOR2D getBoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                             TEXT_STYLE_FLAGS aTextStyle = 0 ) const override;

private:
    // FreeType variables
    static FT_Library mFreeType;
    FT_Face           mFace;
    const int         mFaceSize;
    FT_Face           mSubscriptFace;
    const int         mSubscriptSize;

    int mFaceScaler;
    int mSubscriptFaceScaler;

    // cache for glyphs converted to straight segments
    // key is glyph index (FT_GlyphSlot field glyph_index)
    std::map<unsigned int, GLYPH_POINTS_LIST> mContourCache;

    FT_Error loadFace( const wxString& aFontFileName );

    bool loadFontSimple( const wxString& aFontFileName );

    /**
     * Draw a single line of text. Multiline texts should be split before using the
     * function.
     *
     * @param aText is the text to be drawn.
     * @param aPosition is text position.
     * @param aAngle is text angle (defaults to 0)
     * @return bounding box width/height
     */
    VECTOR2D drawSingleLineText( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                                 const EDA_ANGLE& aAngle = EDA_ANGLE() ) const override;

    VECTOR2D drawMarkup( KIGFX::GAL* aGal, const MARKUP::MARKUP_NODE& aNode,
                         const VECTOR2D& aPosition, const EDA_ANGLE& aAngle,
                         TEXT_STYLE_FLAGS aTextStyle = 0, int aLevel = 0 ) const;

    VECTOR2D drawMarkup( std::vector<SHAPE_POLY_SET>& aGlyphs, const MARKUP::MARKUP_NODE& aNode,
                         const VECTOR2D& aPosition, const VECTOR2D& aGlyphSize, bool aIsMirrored,
                         const EDA_ANGLE& aAngle, TEXT_STYLE_FLAGS aTextStyle = 0,
                         int aLevel = 0 ) const;

    BOX2I getBoundingBox( const std::vector<SHAPE_POLY_SET>& aGlyphs ) const;
};
} //namespace KIFONT

#endif // OUTLINE_FONT_H_
