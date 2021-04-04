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
#include <algorithm>
#include <wx/string.h>

#include <utf8.h>
#include <font/text_attributes.h>

namespace KIGFX
{
class GAL;
}

enum TEXT_STYLE
{
    BOLD = 1,
    ITALIC = 1 << 1,
    SUBSCRIPT = 1 << 2,
    SUPERSCRIPT = 1 << 3,
    OVERBAR = 1 << 4
};

using TEXT_STYLE_FLAGS = unsigned int;

inline bool IsBold( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::BOLD;
}

inline bool IsItalic( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::ITALIC;
}

inline bool IsSuperscript( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::SUPERSCRIPT;
}

inline bool IsSubscript( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::SUBSCRIPT;
}

inline bool IsOverbar( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::OVERBAR;
}

std::string TextStyleAsString( TEXT_STYLE_FLAGS aFlags );

namespace KIFONT
{
/**
   FONT is an abstract base class for both outline and stroke fonts
 */
class FONT
{
public:
    explicit FONT();

    virtual ~FONT()
    {
        //
    }

    virtual bool IsStroke() const { return false; }
    virtual bool IsOutline() const { return false; }

    static FONT* GetFont( const wxString& aFontName = "" );
    static bool  IsOutline( const wxString& aFontName );

    const wxString&    Name() const;
    inline const char* NameAsToken() const { return Name().utf8_str().data(); }

    /**
     * Load a font.
     *
     * @param aFontName is the name of the font. If empty, Newstroke is loaded by default.
     * @return True, if the font was successfully loaded, else false.
     */
    virtual bool LoadFont( const wxString& aFontName = "" ) = 0;

    virtual void DrawText( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                           const TEXT_ATTRIBUTES& aAttributes ) const;

    /**
     * Draw a string.
     *
     * @param aGal is the graphics context.
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aRotationAngle is the text rotation angle
     * @return bounding box width/height
     */
    virtual VECTOR2D Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                           const VECTOR2D& aOrigin, const EDA_ANGLE& aRotationAngle ) const = 0;

    VECTOR2D Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                   double aRotationAngle ) const
    {
        return Draw( aGal, aText, aPosition, VECTOR2D( 0, 0 ),
                     EDA_ANGLE( aRotationAngle, EDA_ANGLE::RADIANS ) );
    }

    /**
     * Draw a string.
     *
     * @param aGal is the graphics context
     * @param aText is the text string
     * @param aPosition is the text position in world coordinates
     * @param aParse is true is aText should first be parsed for variable substition etc.,
     *     otherwise false (default is false)
     * @return bounding box width/height
     */
    VECTOR2D DrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                         bool aParse, const VECTOR2D& aGlyphSize,
                         const TEXT_ATTRIBUTES& aAttributes ) const
    {
        return doDrawString( aGal, aText, aPosition, aParse, aGlyphSize, aAttributes,
                             aAttributes.GetAngle() );
    }

    VECTOR2D DrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                         bool aParse, const VECTOR2D& aGlyphSize, const EDA_ANGLE& aAngle ) const
    {
        return doDrawString( aGal, aText, aPosition, aParse, aGlyphSize, TEXT_ATTRIBUTES(),
                             aAngle );
    }

    VECTOR2D DrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition )
    {
        return DrawString( aGal, aText, aPosition, false, VECTOR2D( 0, 0 ), TEXT_ATTRIBUTES() );
    }

    VECTOR2D DrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                         bool aParse )
    {
        return DrawString( aGal, aText, aPosition, aParse, VECTOR2D( 0, 0 ), TEXT_ATTRIBUTES() );
    }

    VECTOR2D DrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                         bool aParse, const VECTOR2D& aGlyphSize )
    {
        return DrawString( aGal, aText, aPosition, aParse, aGlyphSize, TEXT_ATTRIBUTES() );
    }

    /**
     * Compute the boundary limits of aText (the bounding box of all shapes).
     *
     * The overbar and attributes are not taken in account, '~' characters are skipped.
     *
     * @return a VECTOR2D giving the width and height of text.
     */
    virtual VECTOR2D StringBoundaryLimits( const KIGFX::GAL* aGal, const UTF8& aText,
                                           const VECTOR2D& aGlyphSize,
                                           double          aGlyphThickness ) const = 0;

    VECTOR2D StringBoundaryLimits( const UTF8& aText, const VECTOR2D& aGlyphSize,
                                   double aGlyphThickness ) const
    {
        return StringBoundaryLimits( nullptr, aText, aGlyphSize, aGlyphThickness );
    }


#if 0
    virtual VECTOR2D BoundingBox( const KIGFX::GAL* aGal, const UTF8& aText,
                                  const VECTOR2D& aGlyphSize, double aGlyphThickness ) const;
#endif

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
    virtual VECTOR2D ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const = 0;

    VECTOR2D
    BoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize, TEXT_STYLE_FLAGS aTextStyle = 0,
                 const TEXT_ATTRIBUTES& aAttributes = TEXT_ATTRIBUTES() )
    {
        // TODO: add version of BoundingBox with a free angle - only
        // if needed, doesn't look like there are any uses like that
        return getBoundingBox( aString, aGlyphSize, aTextStyle, aAttributes,
                               aAttributes.GetAngle() );
    }

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

    /**
     * Draws a single line of text. Multiline texts should be split before using the
     * function.
     *
     * @param aText is the text to be drawn.
     * @param aPosition is text position.
     * @param aAngle is text angle.
     * @return bounding box width/height
     */
    virtual VECTOR2D drawSingleLineText( KIGFX::GAL* aGal, const UTF8& aText,
                                         const VECTOR2D&  aPosition,
                                         const EDA_ANGLE& aAngle ) const = 0;

    void getLinePositions( const UTF8& aText, const VECTOR2D& aPosition, wxArrayString& aStringList,
                           std::vector<wxPoint>& aPositions, int& aLineCount,
                           std::vector<VECTOR2D>& aBoundingBoxes, const VECTOR2D& aGlyphSize,
                           const TEXT_ATTRIBUTES& aAttributes, const EDA_ANGLE& aAngle ) const;

    virtual VECTOR2D getBoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                                     TEXT_STYLE_FLAGS aTextStyle = 0 ) const = 0;

private:
    static FONT*                     s_defaultFont;
    static std::map<wxString, FONT*> s_fontMap;

    static FONT* getDefaultFont();

    VECTOR2D doDrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                           bool aParse, const VECTOR2D& aGlyphSize,
                           const TEXT_ATTRIBUTES& aAttributes, const EDA_ANGLE& aAngle ) const;
    VECTOR2D getBoundingBox( const UTF8& aText, const VECTOR2D& aGlyphSize,
                             TEXT_STYLE_FLAGS aTextStyle, const TEXT_ATTRIBUTES& aAttributes,
                             const EDA_ANGLE& aAngle ) const;
};
} //namespace KIFONT

#endif // FONT_H_
