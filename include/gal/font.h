/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Font abstract base class
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

#ifndef FONT_H_
#define FONT_H_

#include <map>
#include <wx/string.h>

#include <utf8.h>
#include <eda_text.h>

namespace KIGFX
{
class GAL;

class FONT
{
public:
    explicit FONT();

    virtual ~FONT()
    {
        //
    }

    static FONT* GetFont( const wxString& aFontName = "" );

    /**
     * Load a font.
     *
     * @param aFontName is the name of the font. If empty, Newstroke is loaded by default.
     * @return True, if the font was successfully loaded, else false.
     */
    virtual bool LoadFont( const wxString& aFontName = "" ) = 0;

    /**
     * Draw a string.
     *
     * @param aGal is the graphics context.
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aRotationAngle is the text rotation angle in radians.
     */
    virtual void Draw( GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                       double aRotationAngle ) const = 0;

    /**
     * Compute the boundary limits of aText (the bounding box of all shapes).
     *
     * The overbar and alignment are not taken in account, '~' characters are skipped.
     *
     * @return a VECTOR2D giving the width and height of text.
     */
    virtual VECTOR2D ComputeStringBoundaryLimits( const GAL* aGal, const UTF8& aText,
                                                  const VECTOR2D& aGlyphSize,
                                                  double          aGlyphThickness ) const = 0;

    /**
     * Compute the vertical position of an overbar, sometimes used in texts.
     *
     * This is the distance between the text base line and the overbar.
     *
     * @param aGlyphHeight is the height (vertical size) of the text.
     * @return the relative position of the overbar axis.
     */
    virtual double ComputeOverbarVerticalPosition( double aGlyphHeight ) const = 0;

    /**
     * Compute the distance (interline) between 2 lines of text (for multiline texts).
     *
     * @param aGlyphHeight is the height (vertical size) of the text.
     * @return the interline.
     */
    virtual double GetInterline( double aGlyphHeight ) const = 0;

    /**
     * Compute the X and Y size of a given text. The text is expected to be
     * a only one line text.
     *
     * @param aText is the text string (one line).
     * @return the text size.
     */
    virtual VECTOR2D ComputeTextLineSize( const GAL* aGal, const UTF8& aText ) const = 0;

protected:
    wxString m_fontName;     ///< Font name
    wxString m_fontFileName; ///< Font file name

    /**
     * Returns number of lines for a given text.
     *
     * @param aText is the text to be checked.
     * @return unsigned - The number of lines in aText.
     */
    inline unsigned linesCount( const UTF8& aText ) const
    {
        if( aText.empty() )
            return 0; // std::count does not work well with empty strings
        else
            // aText.end() - 1 is to skip a newline character that is potentially at the end
            return std::count( aText.begin(), aText.end() - 1, '\n' ) + 1;
    }

private:
    static FONT* defaultFont;
};

typedef std::map<wxString, FONT*> FONT_MAP;

} // namespace KIGFX

#endif // FONT_H_
