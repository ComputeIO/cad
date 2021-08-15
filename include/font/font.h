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

#include <iostream>
#include <map>
#include <algorithm>
#include <wx/string.h>

#include <utf8.h>
#include <font/glyph.h>
#include <font/text_attributes.h>
#include <parser/markup_parser.h>

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
    virtual bool IsBold() const { return false; }
    virtual bool IsItalic() const { return false; }

    static FONT* GetFont( const wxString& aFontName = "", bool aBold = false,
                          bool aItalic = false );
    static bool  IsOutline( const wxString& aFontName );

    const wxString&    Name() const;
    inline const char* NameAsToken() const { return Name().utf8_str().data(); }

    /**
     * Load a font.
     *
     * @param aFontName is the name of the font. If empty, Newstroke is loaded by default.
     * @param aBool is true if a bold version of the font should be loaded,
     *        otherwise false (default).
     * @param aItalic is true if an italic version of the font should be loaded,
     *        otherwise false (default).
     * @return True, if the font was successfully loaded, else false.
     */
    virtual bool LoadFont( const wxString& aFontName = "", bool aBold = false,
                           bool aItalic = false ) = 0;

    /**
     * Draw a string.
     *
     * @param aGal is the graphics context.
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aRotationAngle is the text rotation angle
     * @return bounding box
     */
    VECTOR2D Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                   const VECTOR2D& aOrigin, const TEXT_ATTRIBUTES& aAttributes ) const;

    VECTOR2D Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                   const TEXT_ATTRIBUTES& aAttributes ) const
    {
        return Draw( aGal, aText, aPosition, VECTOR2D( 0, 0 ), aAttributes );
    }

    VECTOR2D Draw( KIGFX::GAL* aGal, const EDA_TEXT& aText, const VECTOR2D& aPosition ) const;
    VECTOR2D Draw( KIGFX::GAL* aGal, const EDA_TEXT& aText ) const;
    VECTOR2D Draw( KIGFX::GAL* aGal, const EDA_TEXT* aText ) const { return Draw( aGal, *aText ); }

    virtual void DrawText( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                           const TEXT_ATTRIBUTES& aAttributes ) const;

    /**
     * Draw a string.
     *
     * @param aGal is the graphics context
     * @param aText is the text string
     * @param aPosition is the text position in world coordinates
     * @param aParse is true is aText should first be parsed for variable substition etc.,
     *     otherwise false (default is false)
     * @return bounding box
     */
    VECTOR2D DrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                         bool aParse, const VECTOR2D& aGlyphSize,
                         const TEXT_ATTRIBUTES& aAttributes ) const
    {
        return doDrawString( aGal, aText, aPosition, aParse, aGlyphSize, aAttributes );
    }

    VECTOR2D DrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                         bool aParse, const VECTOR2D& aGlyphSize, const EDA_ANGLE& aAngle ) const
    {
        TEXT_ATTRIBUTES attributes;
        attributes.SetAngle( aAngle );
        return doDrawString( aGal, aText, aPosition, aParse, aGlyphSize, attributes );
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
     * @param aLineSpacing is the line spacing multiplier (defaults to 1.0)
     * @return the interline.
     */
    virtual double GetInterline( double aGlyphHeight, double aLineSpacing = 1.0 ) const = 0;

    /**
     * Compute the X and Y size of a given text. The text is expected to be
     * a only one line text.
     *
     * @param aText is the text string (one line).
     * @return the text size.
     */
    virtual VECTOR2D ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const = 0;

    VECTOR2D BoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                          TEXT_STYLE_FLAGS       aTextStyle = 0,
                          const TEXT_ATTRIBUTES& aAttributes = TEXT_ATTRIBUTES() )
    {
        // TODO: add version of BoundingBox with a free angle - only
        // if needed, doesn't look like there are any uses like that
        return getBoundingBox( aString, aGlyphSize, aTextStyle, aAttributes );
    }

    VECTOR2D BoundingBox( const EDA_TEXT& aText );

    /**
     * Convert text string to polygon (outline font) or polyline (stroke font).
     *
     * @param aBoundingBox pointer to a BOX2I that will set to the bounding box, or nullptr
     * @param aText text to convert to polygon/polyline
     * @param aGlyphSize glyph size
     * @param aPosition position of text (cursor position before this text)
     * @param aAngle text angle (default 0)
     * @param aIsMirrored is text mirrored? (default false)
     * @param aTextStyle text style flags (default 0 = no flags)
     * @return text cursor position after this text
     */
    virtual VECTOR2I GetTextAsPolygon( BOX2I* aBoundingBox, GLYPH_LIST& aGlyphs, const UTF8& aText,
                                       const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                                       const EDA_ANGLE& aAngle = EDA_ANGLE::ANGLE_0,
                                       bool             aIsMirrored = false,
                                       TEXT_STYLE_FLAGS aTextStyle = 0 ) const = 0;

    /**
     * Convert text string to polygon (outline font) or polyline (stroke font).
     * Bounding box is not returned.
     * Parameters: see above.
     */
    VECTOR2I GetTextAsPolygon( GLYPH_LIST& aGlyphs, const UTF8& aText, const VECTOR2D& aGlyphSize,
                               const wxPoint&   aPosition,
                               const EDA_ANGLE& aAngle = EDA_ANGLE::ANGLE_0,
                               bool aIsMirrored = false, TEXT_STYLE_FLAGS aTextStyle = 0 ) const
    {
        return GetTextAsPolygon( nullptr, aGlyphs, aText, aGlyphSize, aPosition, aAngle,
                                 aIsMirrored, aTextStyle );
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
     * @param aGal is a pointer to the graphics abstraction layer, or nullptr (nothing is drawn)
     * @param aBoundingBox is a pointer to a BOX2I variable which will be set to the bounding box, or nullptr
     * @param aText is the text to be drawn.
     * @param aPosition is text position.
     * @param aAngle is text angle.
     * @param aIsMirrored is true if text should be drawn mirrored, false otherwise.
     * @return new cursor position
     */
    virtual VECTOR2D drawSingleLineText( KIGFX::GAL* aGal, BOX2I* aBoundingBox, const UTF8& aText,
                                         const VECTOR2D&  aPosition,
                                         const EDA_ANGLE& aAngle = EDA_ANGLE(),
                                         bool aIsMirrored = false ) const;

    void getLinePositions( const UTF8& aText, const VECTOR2D& aPosition, wxArrayString& aStringList,
                           std::vector<wxPoint>& aPositions, int& aLineCount,
                           std::vector<VECTOR2D>& aBoundingBoxes, const VECTOR2D& aGlyphSize,
                           const TEXT_ATTRIBUTES& aAttributes ) const;

    virtual VECTOR2D getBoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                                     TEXT_STYLE_FLAGS aTextStyle = 0 ) const = 0;

    VECTOR2D drawMarkup( BOX2I* aBoundingBox, GLYPH_LIST& aGlyphs, const MARKUP::MARKUP_NODE& aNode,
                         const VECTOR2D& aPosition, const VECTOR2D& aGlyphSize,
                         const EDA_ANGLE& aAngle = EDA_ANGLE::ANGLE_0, bool aIsMirrored = false,
                         TEXT_STYLE_FLAGS aTextStyle = 0, int aLevel = 0 ) const;
    VECTOR2D drawMarkup( GLYPH_LIST& aGlyphs, const MARKUP::MARKUP_NODE& aNode,
                         const VECTOR2D& aPosition, const VECTOR2D& aGlyphSize,
                         const EDA_ANGLE& aAngle = EDA_ANGLE::ANGLE_0, bool aIsMirrored = false,
                         TEXT_STYLE_FLAGS aTextStyle = 0, int aLevel = 0 ) const
    {
        return drawMarkup( nullptr, aGlyphs, aNode, aPosition, aGlyphSize, aAngle, aIsMirrored,
                           aTextStyle, aLevel );
    }

    static wxString getFontNameForFontconfig( const wxString& aFontName, bool aBold, bool aItalic );

private:
    static FONT*                     s_defaultFont;
    static std::map<wxString, FONT*> s_fontMap;

    static FONT* getDefaultFont();

    VECTOR2D doDrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                           bool aParse, const VECTOR2D& aGlyphSize,
                           const TEXT_ATTRIBUTES& aAttributes ) const;
    VECTOR2D getBoundingBox( const UTF8& aText, const VECTOR2D& aGlyphSize,
                             TEXT_STYLE_FLAGS       aTextStyle,
                             const TEXT_ATTRIBUTES& aAttributes ) const;
};
} //namespace KIFONT

inline std::ostream& operator<<(std::ostream& os, const KIFONT::FONT& aFont)
{
    os << "[Font \"" << aFont.Name() << "\"" << ( aFont.IsStroke() ? " stroke" : "" )
       << ( aFont.IsOutline() ? " outline" : "" ) << ( aFont.IsBold() ? " bold" : "" )
       << ( aFont.IsItalic() ? " italic" : "" ) << "]";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const KIFONT::FONT* aFont)
{
    os << *aFont;
    return os;
}

#endif // FONT_H_
