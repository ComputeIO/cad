/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * Stroke font class
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

#ifndef STROKE_FONT_H_
#define STROKE_FONT_H_

#include <map>
#include <deque>
#include <algorithm>
#include <utf8.h>
#include <math/box2.h>
#include <font/font.h>
#include <font/outline_decomposer.h>

namespace KIGFX
{
class GAL;
}

namespace KIFONT
{
/**
 * Implement a stroke font drawing.
 *
 * A stroke font is composed of lines.
 */
class STROKE_FONT : public FONT
{
public:
    /// Constructor
    STROKE_FONT();

    bool IsStroke() const override { return true; }

    /**
     * Load a font.
     *
     * @param aFontName is the name of the font. If empty, Newstroke is loaded by default.
     * @return True, if the font was successfully loaded, else false.
     */
    bool LoadFont( const wxString& aFontName = "", bool aBold = false,
                   bool aItalic = false ) override;

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
    double GetInterline( double aGlyphHeight, double aLineSpacing = 1.0 ) const override;

    /**
     * Compute the X and Y size of a given text. The text is expected to be
     * a only one line text.
     *
     * @param aText is the text string (one line).
     * @return the text size.
     */
    VECTOR2D ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const override;

    VECTOR2I GetTextAsPolygon( BOX2I* aBoundingBox, GLYPH_LIST& aGlyphs, const UTF8& aText,
                               const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                               const EDA_ANGLE& aAngle, bool aIsMirrored,
                               TEXT_STYLE_FLAGS aTextStyle = 0 ) const override;

protected:
    VECTOR2D getBoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                             TEXT_STYLE_FLAGS aTextStyle = 0 ) const override;

private:
    /**
     * Load the new stroke font.
     *
     * @param aNewStrokeFont is the pointer to the font data.
     * @param aNewStrokeFontSize is the size of the font data.
     * @return True, if the font was successfully loaded, else false.
     */
    bool loadNewStrokeFont( const char* const aNewStrokeFont[], int aNewStrokeFontSize );

    /**
     * Load a Hershey font.
     *
     * @param aHersheyFontName is the name of the font (example: "futural")
     * @return true if the font was successfully loaded, else false.
     */
    bool loadHersheyFont( const wxString& aHersheyFontName );

    /**
     * Compute the vertical position of an overbar, sometimes used in texts.
     * This is the distance between the text base line and the overbar.
     * @return the relative position of the overbar axis.
     */
    double computeOverbarVerticalPosition( const KIGFX::GAL* aGal ) const;
    double computeUnderlineVerticalPosition( const KIGFX::GAL* aGal ) const;

    /**
     * Compute the bounding box of a given glyph.
     *
     * @param aGlyph is the glyph.
     * @param aGlyphWidth is the x-component of the bounding box size.
     * @return is the complete bounding box size.
     */
    //BOX2D computeBoundingBox( std::shared_ptr<GLYPH> aGlyph, double aGlyphWidth ) const;

    /**
     * Process a string representing a Hershey font glyph. Not used for Newstroke font
     * as the format is slightly different from the original Hershey format.
     *
     * @param aGlyphString String containing the glyph data
     * @return newly created GLYPH
     */
    std::shared_ptr<GLYPH> processGlyph( std::string aGlyphString );

    const GLYPH_LIST*              m_glyphs;             ///< Glyph list
    const GLYPH_BOUNDING_BOX_LIST* m_glyphBoundingBoxes; ///< Bounding boxes of the glyphs
    double                         m_maxGlyphWidth;

    ///< Factor that determines relative vertical position of the overbar.
    static constexpr double OVERBAR_POSITION_FACTOR = 1.33;
    static constexpr double UNDERLINE_POSITION_FACTOR = 0.41;

    ///< Factor that determines relative line width for bold text.
    static constexpr double BOLD_FACTOR = 1.3;

    ///< Scale factor for a glyph
    static constexpr double STROKE_FONT_SCALE = 1.0 / 21.0;

    ///< Tilt factor for italic style (the is is the scaling factor
    ///< on dY relative coordinates to give a tilt shape
    static constexpr double ITALIC_TILT = 1.0 / 8;

    ///< Factor that determines the pitch between 2 lines.
    static constexpr double INTERLINE_PITCH_RATIO = 1.61;

    static constexpr int FONT_OFFSET = -10;
};
} //namespace KIFONT

#endif // STROKE_FONT_H_
