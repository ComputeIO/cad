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
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include <harfbuzz/hb.h>
#include <font/font.h>

typedef struct
{
    POINTS         points;
    int            winding;
    FT_Orientation orientation;
} CONTOUR;

typedef std::vector<CONTOUR> CONTOURS;


/**
 * Class OUTLINE_FONT implements outline font drawing.
 */
class OUTLINE_FONT : public FONT
{
public:
    OUTLINE_FONT();

    bool IsOutline() const override { return true; }

    /**
     * Load an outline font. TrueType (.ttf) and OpenType (.otf)
     * are supported.
     * @param aFontFileName is the name of the font file
     * @return true if the font was successfully loaded, otherwise false.
     */
    bool LoadFont( const wxString& aFontFileName ) override;

    /**
     * Draw a string.
     *
     * @param aGal
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aRotationAngle is the text rotation angle in radians.
     */
    void Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
               double aRotationAngle ) override;

    /**
     * Compute the boundary limits of aText (the bounding box of all shapes).
     *
     * The overbar and alignment are not taken in account, '~' characters are skipped.
     *
     * @return a VECTOR2D giving the width and height of text.
     */
    VECTOR2D ComputeStringBoundaryLimits( const KIGFX::GAL* aGal, const UTF8& aText,
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
     * @return the interline.
     */
    double GetInterline( double aGlyphHeight ) const override;

    /**
     * Compute the X and Y size of a given text. The text is expected to be
     * a only one line text.
     *
     * @param aText is the text string (one line).
     * @return the text size.
     */
    VECTOR2D ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const override;

    void GetTextAsPolygon( std::vector<SHAPE_POLY_SET>& aGlyphs, const UTF8& aText,
                           const VECTOR2D& aGlyphSize, bool aIsMirrored ) const;

private:
    // FreeType variables
    static FT_Library mFreeType;
    FT_Face           mFace;

    // cache for glyphs converted to straight segments
    // key is glyph index (FT_GlyphSlot field glyph_index)
    std::map<unsigned int, POINTS_LIST> mContourCache;

    FT_Error loadFace( const wxString& aFontFileName );

    /**
     * Draws a single line of text. Multiline texts should be split before using the
     * function.
     *
     * @param aText is the text to be drawn.
     */
    void drawSingleLineText( KIGFX::GAL* aGal, const UTF8& aText );

    void outlineToStraightSegments( CONTOURS& aContours, FT_Outline& aOutline ) const;

    /**
     * @return 1 if aContour is in clockwise order, -1 if it is in
     *     counterclockwise order, or 0 if the winding can't be
     *     determined.
     */
    int winding( const POINTS& aContour ) const;

    /**
     * @return true if aContour is a clockwise contour, otherwise
     *     false.
     */
    inline bool isClockwise( const POINTS& aContour ) const { return winding( aContour ) == 1; }

    /**
     * @return true if aContour is a counterclockwise contour,
     *     otherwise false.
     */
    inline bool isCounterclockwise( const POINTS& aContour ) const
    {
        return winding( aContour ) == -1;
    }

    /**
     * Approximate Bezier curve contour with straight segments.
     * @return Winding (clockwise/counterclockwise) of the contour.
     */
    int approximateContour( const POINTS& aPoints, const std::vector<bool>& aPointOnCurve,
                            POINTS& aResult ) const;

    bool approximateBezierCurve( POINTS& result, const POINTS& bezier ) const;

    inline const unsigned int onCurve( char aTags ) const { return aTags & 0x1; }

    inline const unsigned int thirdOrderBezierPoint( char aTags ) const
    {
        return onCurve( aTags ) ? 0 : aTags & 0x2;
    }

    inline const unsigned int secondOrderBezierPoint( char aTags ) const
    {
        return onCurve( aTags ) ? 0 : !thirdOrderBezierPoint( aTags );
    }

    inline const unsigned int hasDropout( char aTags ) const { return aTags & 0x4; }

    inline const unsigned int dropoutMode( char aTags ) const
    {
        return hasDropout( aTags ) ? ( aTags & 0x38 ) : 0;
    }
};

#endif // OUTLINE_FONT_H_
