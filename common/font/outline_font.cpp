/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
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

#include <limits>
#include <kicad_string.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <harfbuzz/hb-ft.h>
#include <bezier_curves.h>
#include <geometry/shape_poly_set.h>
#include <font/outline_font.h>
#include FT_GLYPH_H
#include FT_BBOX_H
#include <trigo.h>
#include <font/fontconfig.h>

using namespace KIFONT;

FT_Library OUTLINE_FONT::mFreeType = nullptr;

OUTLINE_FONT::OUTLINE_FONT() : mFaceSize( 16 ), mSubscriptSize( 10 )
{
    if( !mFreeType )
    {
        //FT_Error ft_error = FT_Init_FreeType( &mFreeType );
        // TODO: handle ft_error
        FT_Init_FreeType( &mFreeType );
    }
}


bool OUTLINE_FONT::LoadFont( const wxString& aFontName )
{
    wxString fontFile;
    bool     r = Fontconfig().FindFont( aFontName, fontFile );
#ifdef DEBUG
    std::cerr << "Fontconfig ";
    if( r )
        std::cerr << "found  [" << fontFile << "]";
    else
        std::cerr << "did not find font";
    std::cerr << " for [" << aFontName << "]" << std::endl;
#endif
    if( r )
    {
        FT_Error e = loadFace( fontFile );
        if( e )
        {
#ifdef DEBUG
            std::cerr << "Could not load [" << fontFile << "]" << std::endl;
#endif
            return false;
        }
        return true;
    }

    return loadFontSimple( aFontName );
}


bool OUTLINE_FONT::loadFontSimple( const wxString& aFontFileName )
{
    wxFileName fontFile( aFontFileName );
    wxString   fileName = fontFile.GetFullPath();
    // TODO: handle ft_error properly (now we just return false if load does not succeed)
    FT_Error ft_error = loadFace( fileName );

    if( ft_error )
    {
        // Try user dir
        fontFile.SetExt( "otf" );
        fontFile.SetPath( Pgm().GetSettingsManager().GetUserSettingsPath() + wxT( "/fonts" ) );
        fileName = fontFile.GetFullPath();

        if( wxFile::Exists( fileName ) )
        {
            ft_error = loadFace( fileName );
        }
        else
        {
            fontFile.SetExt( "ttf" );
            fileName = fontFile.GetFullPath();
            if( wxFile::Exists( fileName ) )
            {
                ft_error = loadFace( fileName );
            }
        }
    }

    if( ft_error == FT_Err_Unknown_File_Format )
    {
        std::cerr << "The font file " << fileName << " could be opened and read, "
                  << "but it appears that its font format is unsupported." << std::endl;
    }
    else if( ft_error )
    {
        std::cerr << "ft_error " << ft_error << std::endl;
        return false;
    }
    else
    {
        m_fontName = aFontFileName;
        m_fontFileName = fileName;
    }

    return true;
}


FT_Error OUTLINE_FONT::loadFace( const wxString& aFontFileName )
{
    mFaceScaler = mFaceSize * 64;
    mSubscriptFaceScaler = mSubscriptSize * 64;

    // TODO: check that going from wxString to char* with UTF-8
    // conversion for filename makes sense on any/all platforms
    FT_Error e = FT_New_Face( mFreeType, aFontFileName.mb_str( wxConvUTF8 ), 0, &mFace );
    if( !e )
    {
        FT_Select_Charmap( mFace, FT_Encoding::FT_ENCODING_UNICODE );
        FT_Set_Char_Size( mFace, 0, mFaceScaler, 0, 0 );

        e = FT_New_Face( mFreeType, aFontFileName.mb_str( wxConvUTF8 ), 0, &mSubscriptFace );
        if( !e )
        {
            FT_Select_Charmap( mSubscriptFace, FT_Encoding::FT_ENCODING_UNICODE );
            FT_Set_Char_Size( mSubscriptFace, 0, mSubscriptFaceScaler, 0, 0 );

            m_fontName = wxString( mFace->family_name );
        }
    }
#ifdef DEBUG
    std::cerr << "OUTLINE_FONT::loadFace( " << aFontFileName << " ) "
              << ( e ? "did not load" : "loaded" ) << std::endl;
#endif
    return e;
}


/**
 * Draw a string.
 *
 * @param aGal
 * @param aText is the text to be drawn.
 * @param aPosition is the text position in world coordinates.
 * @param aAngle is the text rotation angle
 */
VECTOR2D OUTLINE_FONT::Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                             const VECTOR2D& aOrigin, const EDA_ANGLE& aAngle ) const
{
#ifdef DEBUG
    std::cerr << "OUTLINE_FONT::Draw( aGal, \"" << aText << "\", " << aPosition << ", " << aAngle
              << " ) const\n";
#endif
    if( aText.empty() )
        return VECTOR2D( 0, 0 );

    // FT_Select_Charmap( mFace, FT_Encoding::FT_ENCODING_UNICODE );

    // FT_Set_Char_Size(
    //   <handle to face object>,
    //   <char_width in 1/64th of points>,
    //   <char_height in 1/64th of points>,
    //   <horizontal device resolution>,
    //   <vertical device resolution>
    // )
    // handle == variable (reference)
    // If only one of char_width/char_height == 0, it defaults to "same as the non-zero one"
    // Ditto if only one of horizontal/vertical resolution == 0
    // If both horizontal/vertical resolution == 0, 72 dpi is default
    //
    // 16 * 64 is chosen as an arbitrary default for width/height
    // TODO: come up with a non-arbitrary default (and/or use a better
    // value for H/V reso)

    //FT_Set_Char_Size( mFace, 0, 16 * 64, 0, 0 );

    if( aGal )
    {
        // Context needs to be saved before any transformations
        aGal->Save();
        aGal->Translate( aPosition );
        aGal->Rotate( aAngle.Invert().AsRadians() );
    }

    VECTOR2D bbox = drawSingleLineText( aGal, aText, wxPoint( 0, 0 ), EDA_ANGLE() );

    if( aGal )
        aGal->Restore();

    return bbox;
}


/**
 * Compute the boundary limits of aText (the bounding box of all shapes).
 *
 * The overbar and alignment are not taken in account, '~' characters are skipped.
 *
 * @return a VECTOR2D giving the width and height of text.
 */
VECTOR2D OUTLINE_FONT::StringBoundaryLimits( const KIGFX::GAL* aGal, const UTF8& aText,
                                             const VECTOR2D& aGlyphSize,
                                             double          aGlyphThickness ) const
{
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, aText.c_str(), -1, 0, -1 );

    // guess direction, script, and language based on contents
    hb_buffer_guess_segment_properties( buf );

    unsigned int     glyphCount;
    hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos( buf, &glyphCount );
    hb_font_t*       referencedFont = hb_ft_font_create_referenced( mFace );
    //hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( buf, &glyphCount );

    hb_ft_font_set_funcs( referencedFont );
    hb_shape( referencedFont, buf, NULL, 0 );

    int width = 0;
    int height = mFace->size->metrics.height;

    FT_UInt previous;
    for( int i = 0; i < (int) glyphCount; i++ )
    {
        //hb_glyph_position_t& pos = glyphPos[i];
        int codepoint = glyphInfo[i].codepoint;

        if( i > 0 )
        {
            FT_Vector delta;
            FT_Get_Kerning( mFace, previous, codepoint, FT_KERNING_DEFAULT, &delta );
            width += delta.x >> 6;
        }

        FT_Load_Glyph( mFace, codepoint, FT_LOAD_NO_BITMAP );
        FT_GlyphSlot glyph = mFace->glyph;

        width += glyph->advance.x >> 6;
        previous = codepoint;
    }

    VECTOR2D r = VECTOR2D( width * mFaceScaler, height * mFaceScaler );
#ifdef DEBUG
    std::cerr << "outline string boundary limits " << r << std::endl;
#endif
    return r;
}


/**
 * Compute the vertical position of an overbar, sometimes used in texts.
 *
 * This is the distance between the text base line and the overbar.
 *
 * @param aGlyphHeight is the height (vertical size) of the text.
 * @return the relative position of the overbar axis.
 */
double OUTLINE_FONT::ComputeOverbarVerticalPosition( double aGlyphHeight ) const
{
    // TODO: dummy to make this compile! not used
    return aGlyphHeight;
}


/**
 * Compute the distance (interline) between 2 lines of text (for multiline texts).
 *
 * @param aGlyphHeight is the height (vertical size) of the text.
 * @return the interline.
 */
double OUTLINE_FONT::GetInterline( double aGlyphHeight ) const
{
    if( aGlyphHeight < 0 )
    {
        return ( mFace->size->metrics.ascender - mFace->size->metrics.descender ) >> 6;
    }
    else
    {
        // TODO
        return aGlyphHeight;
    }
}


/**
 * Compute the X and Y size of a given text. The text is expected to be
 * a only one line text.
 *
 * @param aText is the text string (one line).
 * @return the text size.
 */
VECTOR2D OUTLINE_FONT::ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const
{
    return StringBoundaryLimits( aGal, aText, aGal->GetGlyphSize(), 0.0 );
}


static bool contourIsFilled( const CONTOUR& c )
{
    switch( c.orientation )
    {
    case FT_ORIENTATION_TRUETYPE: return c.winding == 1;

    case FT_ORIENTATION_POSTSCRIPT: return c.winding == -1;

    default: break;
    }

    return false;
}


static bool contourIsHole( const CONTOUR& c )
{
    return !contourIsFilled( c );
}


/**
   @return position of cursor for drawing next substring
 */
VECTOR2D OUTLINE_FONT::drawMarkup( KIGFX::GAL* aGal, const MARKUP::MARKUP_NODE& aNode,
                                   const VECTOR2D& aPosition, const EDA_ANGLE& aAngle,
                                   TEXT_STYLE_FLAGS aTextStyle, int aLevel ) const
{
    VECTOR2D nextPosition = aPosition;

    TEXT_STYLE_FLAGS textStyle = aTextStyle;

    if( !aNode->is_root() )
    {
        if( aNode->isSubscript() )
        {
            textStyle = TEXT_STYLE::SUBSCRIPT;
        }
        else if( aNode->isSuperscript() )
        {
            textStyle = TEXT_STYLE::SUPERSCRIPT;
        }

        if( aNode->isOverbar() )
        {
            textStyle |= TEXT_STYLE::OVERBAR;
        }

#ifdef DEBUG
        std::cerr << "OUTLINE_FONT::drawMarkup( [aGal], " << aNode->asString() << ", " << aPosition
                  << ", " << aAngle << ", " << TextStyleAsString( aTextStyle ) << ", " << aLevel
                  << " ) const; textStyle " << TextStyleAsString( textStyle ) << std::endl;
#endif
        if( aNode->has_content() )
        {
            std::string                 txt = aNode->string();
            std::vector<SHAPE_POLY_SET> glyphs;
            wxPoint                     pt( aPosition.x, aPosition.y );
            // TODO better default for glyphSize; this is just a guess based on absolutely nothing
            VECTOR2D glyphSize = aGal ? aGal->GetGlyphSize() : VECTOR2D( 16.0, 16.0 );
            bool     mirrored = aGal ? aGal->IsTextMirrored() : false;

            nextPosition =
                    GetTextAsPolygon( glyphs, txt, glyphSize, pt, aAngle, mirrored, textStyle );

            if( aGal )
            {
                VECTOR2I textSize = nextPosition;
                double   lineHeight = textSize.y;
                double   xAdjust = 0.0;
                double   yAdjust = 0.0;
                switch( aGal->GetHorizontalJustify() )
                {
                case GR_TEXT_HJUSTIFY_LEFT: break;
                case GR_TEXT_HJUSTIFY_RIGHT: xAdjust = -textSize.x; break;
                case GR_TEXT_HJUSTIFY_CENTER:
                default: xAdjust = -textSize.x / 2.0;
                }

                switch( aGal->GetVerticalJustify() )
                {
                case GR_TEXT_VJUSTIFY_TOP: yAdjust = lineHeight / 2; break;
                case GR_TEXT_VJUSTIFY_BOTTOM: yAdjust = -lineHeight / 2; break;
                case GR_TEXT_VJUSTIFY_CENTER:
                default: break;
                }

                aGal->Translate( VECTOR2D( xAdjust, yAdjust ) );

                for( auto glyph : glyphs )
                    aGal->DrawGlyph( glyph );
            }
        }
    }

    for( const auto& child : aNode->children )
    {
        nextPosition = drawMarkup( aGal, child, nextPosition, aAngle, textStyle, aLevel + 1 );
    }

#ifdef DEBUG
    std::cerr << " returns " << nextPosition << std::endl;
#endif
    return nextPosition;
}


/**
   @return position of cursor for drawing next substring
 */
VECTOR2D OUTLINE_FONT::drawMarkup( std::vector<SHAPE_POLY_SET>& aGlyphs,
                                   const MARKUP::MARKUP_NODE& aNode, const VECTOR2D& aPosition,
                                   const VECTOR2D& aGlyphSize, bool aIsMirrored,
                                   const EDA_ANGLE& aAngle, TEXT_STYLE_FLAGS aTextStyle,
                                   int aLevel ) const
{
    VECTOR2D nextPosition = aPosition;

    TEXT_STYLE_FLAGS textStyle = aTextStyle;

    if( !aNode->is_root() )
    {
        if( aNode->isSubscript() )
        {
            textStyle = TEXT_STYLE::SUBSCRIPT;
        }
        else if( aNode->isSuperscript() )
        {
            textStyle = TEXT_STYLE::SUPERSCRIPT;
        }

        if( aNode->isOverbar() )
        {
            textStyle |= TEXT_STYLE::OVERBAR;
        }

#ifdef DEBUG
        std::cerr << "OUTLINE_FONT::drawMarkup( [aGlyphs], " << aNode->asString() << ", "
                  << aPosition << ", " << aGlyphSize << ", " << aAngle << ", "
                  << TextStyleAsString( aTextStyle ) << ", " << aLevel << " ) const; textStyle "
                  << TextStyleAsString( textStyle ) << std::endl;
#endif
        if( aNode->has_content() )
        {
            std::string txt = aNode->string();
            //std::vector<SHAPE_POLY_SET> glyphs;
            wxPoint pt( aPosition.x, aPosition.y );

            nextPosition = GetTextAsPolygon( aGlyphs, txt, aGlyphSize, pt, aAngle, aIsMirrored,
                                             textStyle );
        }
    }

    for( const auto& child : aNode->children )
    {
        nextPosition = drawMarkup( aGlyphs, child, nextPosition, aGlyphSize, aIsMirrored, aAngle,
                                   textStyle, aLevel + 1 );
    }

#ifdef DEBUG
    std::cerr << " returns " << nextPosition << std::endl;
#endif
    return nextPosition;
}


VECTOR2D OUTLINE_FONT::drawSingleLineText( KIGFX::GAL* aGal, const UTF8& aText,
                                           const VECTOR2D&  aPosition,
                                           const EDA_ANGLE& aAngle ) const
{
#ifdef DEBUG
    if( debugMe( aText ) )
        std::cerr << "OUTLINE_FONT::drawSingleLineText( aGal, \"" << aText << "\", " << aPosition
                  << " )";
#endif
#if 0
    if( agal && aGal->IsOpenGlEngine() )
    {
        // draw with OpenGL routines to get antialiased pixmaps
        // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //aGal->GetFreeType()->Render( this, aText );
        //this should go in drawMarkup()
    }
    // for now let's just do what we do for all GAL types
#endif

#if 1
    MARKUP::MARKUP_PARSER markupParser( aText );
    auto                  markupRoot = markupParser.Parse();

    VECTOR2D position( 0, 0 ); //position(aPosition);
    drawMarkup( aGal, markupRoot, position, aAngle );

    return position;
#else
    // transform glyph outlines to straight segments, then fill
    std::vector<SHAPE_POLY_SET> glyphs;
    wxPoint                     pt( aPosition.x, aPosition.y );

    VECTOR2I textSize = GetTextAsPolygon( glyphs, aText, aGal->GetGlyphSize(), pt, aAngle,
                                          aGal->IsTextMirrored() );
    double   lineHeight = textSize.y;
    double   xAdjust = 0.0;
    double   yAdjust = 0.0;
    switch( aGal->GetHorizontalJustify() )
    {
    case GR_TEXT_HJUSTIFY_LEFT: break;
    case GR_TEXT_HJUSTIFY_RIGHT: xAdjust = -textSize.x; break;
    case GR_TEXT_HJUSTIFY_CENTER:
    default: xAdjust = -textSize.x / 2.0;
    }

    switch( aGal->GetVerticalJustify() )
    {
    case GR_TEXT_VJUSTIFY_TOP: yAdjust = lineHeight / 2; break;
    case GR_TEXT_VJUSTIFY_BOTTOM: yAdjust = -lineHeight / 2; break;
    case GR_TEXT_VJUSTIFY_CENTER:
    default: break;
    }

    aGal->Translate( VECTOR2D( xAdjust, yAdjust ) );

    //aGal->SetIsFill( true );
    aGal->DrawGlyphs( glyphs );
    //return???
#endif
}


#if 0
static inline int getp2( int a )
{
    int rval = 1;
    while( rval < a )
        rval <<= 1;
    return rval;
}
#endif


VECTOR2I OUTLINE_FONT::GetLinesAsPolygon( std::vector<SHAPE_POLY_SET>& aGlyphs, const UTF8& aText,
                                          const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                                          const TEXT_ATTRIBUTES& aAttributes, bool aIsMirrored,
                                          TEXT_STYLE_FLAGS aTextStyle ) const
{
    wxArrayString         strings;
    std::vector<wxPoint>  positions;
    int                   n;
    VECTOR2I              ret;
    std::vector<VECTOR2D> boundingBoxes;

    wxPoint thePosition( aPosition ); //(0,0);

    getLinePositions( aText, thePosition, strings, positions, n, boundingBoxes, aGlyphSize,
                      aAttributes, aAttributes.GetAngle() );

    for( int i = 0; i < n; i++ )
    {
#if 1
        MARKUP::MARKUP_PARSER markupParser( UTF8( strings.Item( i ) ) );
        auto                  markupRoot = markupParser.Parse();

        //VECTOR2D position( 0, 0 ); //position(aPosition);
        //drawMarkup( aGal, markupRoot, position, aAngle );
        drawMarkup( aGlyphs, markupRoot, positions[i], aGlyphSize, aIsMirrored,
                    aAttributes.GetAngle() );
#else
        ret = GetTextAsPolygon( aGlyphs, strings.Item( i ), aGlyphSize, positions[i],
                                aAttributes.GetAngle(), aIsMirrored, aTextStyle );
#endif
    }
    return ret;
}


VECTOR2I OUTLINE_FONT::GetTextAsPolygon( std::vector<SHAPE_POLY_SET>& aGlyphs, const UTF8& aText,
                                         const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                                         const EDA_ANGLE& aOrientation, bool aIsMirrored,
                                         TEXT_STYLE_FLAGS aTextStyle ) const
{
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, aText.c_str(), -1, 0, -1 );

    // guess direction, script, and language based on contents
    hb_buffer_guess_segment_properties( buf );

    unsigned int         glyphCount;
    hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( buf, &glyphCount );
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( buf, &glyphCount );
    hb_font_t*           referencedFont;

    //const double subscriptAndSuperscriptScaler = 0.5;
    VECTOR2D glyphSize = aGlyphSize;
    FT_Face  face = mFace;
    int      scaler = mFaceScaler;
    if( IsSubscript( aTextStyle ) || IsSuperscript( aTextStyle ) )
    {
        face = mSubscriptFace;
        //scaler = mSubscriptFaceScaler;
    }

    referencedFont = hb_ft_font_create_referenced( face );
    hb_ft_font_set_funcs( referencedFont );
    hb_shape( referencedFont, buf, NULL, 0 );

    const double   mirror_factor = ( aIsMirrored ? 1 : -1 );
    const VECTOR2D scaleFactor( glyphSize.x * mirror_factor / scaler, glyphSize.y / scaler );

#ifdef DEBUG //_GETTEXTASPOLYGON //STROKEFONT
    if( debugMe( aText ) )
    {
        std::cerr << "[OUTLINE_FONT::GetTextAsPolygon( &aGlyphs, \"" << aText << "\", "
                  << aGlyphSize << ", " << aPosition << ", " << aOrientation << ", "
                  << ( aIsMirrored ? "true" : "false" ) << ", " << TextStyleAsString( aTextStyle )
                  << " ) const; glyphCount " << glyphCount << " mirror_factor " << mirror_factor
                  << " scaler " << scaler << " scaleFactor " << scaleFactor << " font " << Name()
                  << " c_str " << aText.c_str() << " ";

        for( unsigned int i = 0; i < glyphCount; i++ )
        {
            hb_glyph_position_t& pos = glyphPos[i];
            int                  codepoint = glyphInfo[i].codepoint;

            FT_Load_Glyph( face, codepoint, FT_LOAD_NO_BITMAP );

            FT_GlyphSlot              glyph = face->glyph;
            static const unsigned int bufsize = 512;
            char                      glyphName[bufsize];
            FT_Get_Glyph_Name( face, glyph->glyph_index, &glyphName[0], bufsize );
            std::cerr << "<glyph " << codepoint << " '" << glyphName << "' pos ";
            std::cerr << pos.x_advance << "," << pos.y_advance << "," << pos.x_offset << ","
                      << pos.y_offset << ">";
        }
        std::cerr << "]" << std::endl;
    }
#endif

    VECTOR2I cursor( 0, 0 );
    VECTOR2I cursorStart( cursor );

    VECTOR2I extentBottomLeft( INT_MAX, INT_MAX );
    VECTOR2I extentTopRight( INT_MIN, INT_MIN );
    VECTOR2I vBottomLeft( INT_MAX, INT_MAX );
    VECTOR2I vTopRight( INT_MIN, INT_MIN );

    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        hb_glyph_position_t& pos = glyphPos[i];
        int                  codepoint = glyphInfo[i].codepoint;

        FT_Load_Glyph( face, codepoint, FT_LOAD_NO_BITMAP );

        FT_GlyphSlot glyph = face->glyph;

        // contours is a collection of all outlines in the glyph;
        // example: glyph for 'o' generally contains 2 contours,
        // one for the glyph outline and one for the hole
        CONTOURS contours;

        OUTLINE_DECOMPOSER decomposer( glyph->outline );
        decomposer.OutlineToSegments( &contours );
#ifdef DEBUG_GETTEXTASPOLYGON
        static const unsigned int bufsize = 512;
        char                      glyphName[bufsize];
        FT_Get_Glyph_Name( face, glyph->glyph_index, &glyphName[0], bufsize );
        std::cerr << "[Glyph '" << glyphName << "' pos ";
        std::cerr << pos.x_advance << "," << pos.y_advance << "," << pos.x_offset << ","
                  << pos.y_offset;
        std::cerr << " codepoint " << codepoint << ", " << glyph->outline.n_contours << " contours "
                  << glyph->outline.n_points << " points]->";
        std::cerr << "{" << contours.size() << " contours ";
        for( int foofaa = 0; foofaa < (int) contours.size(); foofaa++ )
        {
            if( foofaa > 0 )
                std::cerr << "/";
            std::cerr << contours[foofaa].points.size();
        }
        if( contours.size() > 0 )
            std::cerr << " pts";
        std::cerr << "}";
#endif

        SHAPE_POLY_SET                poly;
        std::vector<SHAPE_LINE_CHAIN> holes;
        std::vector<SHAPE_LINE_CHAIN> outlines;

        for( CONTOUR& c : contours )
        {
            GLYPH_POINTS points = c.points;
#undef DEBUG_POINTTRANSFORM
#ifdef DEBUG_POINTTRANSFORM
            int ptCount = 0;
            std::cerr << "c.points.size()==" << c.points.size() << std::endl;
#endif
            SHAPE_LINE_CHAIN shape;

            VECTOR2D offset( aPosition );

            if( IsSubscript( aTextStyle ) )
                offset.y += glyphSize.y * 0.1;
            else if( IsSuperscript( aTextStyle ) )
                offset.y -= glyphSize.y * 0.45;

            for( const VECTOR2D& v : points )
            {
                // Save text extents
                if( vBottomLeft.x > v.x )
                    vBottomLeft.x = v.x;
                if( vBottomLeft.y > v.y )
                    vBottomLeft.y = v.y;
                if( vTopRight.x < v.x )
                    vTopRight.x = v.x;
                if( vTopRight.y < v.y )
                    vTopRight.y = v.y;

#if 0
                VECTOR2D pt( -( v.x + cursor.x ), -( v.y + cursor.y ) );
                wxPoint  scaledPt( pt.x * scaleFactor.x, pt.y * scaleFactor.x );
                RotatePoint( &scaledPt, aOrientation.AsRadians() );
                scaledPt.x += offset.x;
                scaledPt.y += offset.y;
#else
                //VECTOR2D pt( -v.x / scaler, -v.y / scaler );
                VECTOR2D pt( v.x, v.y );
                VECTOR2D ptC( pt.x + cursor.x, pt.y + cursor.y );
                wxPoint  scaledPtOrig( -ptC.x * scaleFactor.x, -ptC.y * scaleFactor.y );
                wxPoint  scaledPt( scaledPtOrig );
                RotatePoint( &scaledPt, aOrientation.AsRadians() );
                scaledPt.x += offset.x;
                scaledPt.y += offset.y;
#endif

#ifdef DEBUG_POINTTRANSFORM
                long int ptx = -( v.x + cursor.x );
                long int pty = -( v.y + cursor.y );
                long int sx = ptx * scaleFactor.x;
                long int sy = pty * scaleFactor.x;
                long int sox = sx + offset.x;
                long int soy = sy + offset.y;

                std::cerr << "(p# " << ptCount << " pt" << pt << " pt" << pt << " ptC " << ptC
                          << " glyphSize " << glyphSize << " scaler " << scaler << " scaleFactor "
                          << scaleFactor << " scaledPt" << scaledPt << " <" << scaledPtOrig << "> v"
                          << v << "[pt" << ptx << "," << pty << " s" << sx << "," << sy << " so"
                          << sox << "," << soy << " offset " << offset << " angle " << aOrientation
                          << "]";
#endif
                if( extentBottomLeft.x > scaledPt.x )
                    extentBottomLeft.x = scaledPt.x;
                if( extentBottomLeft.y > scaledPt.y )
                    extentBottomLeft.y = scaledPt.y;
                if( extentTopRight.x < scaledPt.x )
                    extentTopRight.x = scaledPt.x;
                if( extentTopRight.y < scaledPt.y )
                    extentTopRight.y = scaledPt.y;

                shape.Append( scaledPt.x, scaledPt.y );
                //ptListScaled.push_back( scaledPt );
#ifdef DEBUG_POINTTRANSFORM
                std::cerr << "->n " << shape.PointCount() << ")" << std::endl;
#endif
#ifdef DEBUG_POINTTRANSFORM
                ptCount++;
#endif
            }

#ifdef DEBUG_GETTEXTASPOLYGON
            std::cerr << "(" << ( contourIsHole( c ) ? "hole" : "outline" ) << " shape has "
                      << shape.PointCount() << " pts)";
#endif

            if( contourIsHole( c ) )
            {
                holes.push_back( std::move( shape ) );
            }
            else
            {
                outlines.push_back( std::move( shape ) );
            }
        }

#ifdef DEBUG_GETTEXTASPOLYGON
        if( outlines.size() < 1 )
        {
            std::cerr << "Error: no outlines for [codepoint " << codepoint;
#ifdef FOOBAR //DEBUG
            std::cerr << " " << glyphName;
#endif
            std::cerr << "]!" << std::endl;
        }
#endif

        for( SHAPE_LINE_CHAIN& outline : outlines )
        {
            if( outline.PointCount() )
            {
                outline.SetClosed( true );
                poly.AddOutline( outline );
            }
        }

        int nthHole = 0;
        for( SHAPE_LINE_CHAIN& hole : holes )
        {
            if( hole.PointCount() )
            {
                hole.SetClosed( true );
                VECTOR2I firstPoint = hole.GetPoint( 0 );
                //SHAPE_SIMPLE *outlineForHole = nullptr;
                int nthOutline = -1;
                int n = 0;
                for( SHAPE_LINE_CHAIN& outline : outlines )
                {
                    if( outline.PointInside( firstPoint ) )
                    {
                        //outlineForHole = outline;
                        nthOutline = n;
                        break;
                    }
                    n++;
                }
                if( nthOutline > -1 )
                {
                    poly.AddHole( hole, n );
                }
#ifdef DEBUG_GETTEXTASPOLYGON
                else
                {
                    std::cerr << "Could not find outline for hole " << nthHole << "! ";
                }
#endif
            }
            nthHole++;
        }

        aGlyphs.push_back( poly );

        cursor.x += pos.x_advance;
        cursor.y += pos.y_advance;
    }

#ifdef DEBUG
    std::cerr << "Extents " << extentBottomLeft << ", " << extentTopRight;
    std::cerr << " v " << vBottomLeft << ", " << vTopRight << std::endl;
#endif

    VECTOR2I cursorEnd( cursor );

    if( IsOverbar( aTextStyle ) )
    {
        SHAPE_POLY_SET   overbarGlyph;
        SHAPE_LINE_CHAIN overbar;

        std::vector<VECTOR2D> tmp;
#if 0
        double overbar_bottom = glyphSize.y * y_scaleFactor;
        double overbar_top = glyphSize.y * 1.1 * y_scaleFactor;
        tmp.push_back( VECTOR2D( -cursorStart.x, -cursorStart.y - overbar_bottom ) );
        tmp.push_back( VECTOR2D( -cursorEnd.x, -cursorStart.y - overbar_bottom ) );
        tmp.push_back( VECTOR2D( -cursorEnd.x, -cursorStart.y -  overbar_top ) );
        tmp.push_back( VECTOR2D( -cursorStart.x, -cursorStart.y - overbar_top ) );
        tmp.push_back( VECTOR2D( -cursorStart.x, -cursorStart.y ) );
#else
        /*
        tmp.push_back( VECTOR2D( vBottomLeft.x, vTopRight.y ) );
        tmp.push_back( VECTOR2D( vTopRight.x, vTopRight.y ) );
        tmp.push_back( VECTOR2D( vTopRight.x, vTopRight.y + 30 ) );
        tmp.push_back( VECTOR2D( vBottomLeft.x, vTopRight.y + 30 ) );
        */
        int left = extentBottomLeft.x;
        int right = extentTopRight.x;
        int top = extentBottomLeft.y - 800;
        int barHeight = -800;

        tmp.push_back( VECTOR2D( left, top ) );
        tmp.push_back( VECTOR2D( right, top ) );
        tmp.push_back( VECTOR2D( right, top + barHeight ) );
        tmp.push_back( VECTOR2D( left, top + barHeight ) );
#endif

        for( const VECTOR2D& pt : tmp )
        {
#if 0
            wxPoint scaledPt( pt.x * scaleFactor.x, pt.y * scaleFactor.x );
            RotatePoint( &scaledPt, aOrientation.AsRadians() );
            scaledPt.x += aPosition.x;
            scaledPt.y += aPosition.y;
            overbar.Append( scaledPt );
#else
            overbar.Append( pt );
#endif
#ifdef DEBUG
            std::cerr << overbar.PointCount() << " pts" << std::endl;
#endif
        }

        overbar.SetClosed( true );
        overbarGlyph.AddOutline( overbar );

#ifdef DEBUG
        std::cerr << "overbar has " << overbarGlyph.COutline( 0 ).PointCount() << " ("
                  << overbar.PointCount() << ") pts" << std::endl;
#endif

        aGlyphs.push_back( overbarGlyph );
    }

    VECTOR2I cursorDisplacement( -cursorEnd.x * scaleFactor.x, cursorEnd.y * scaleFactor.y );
#ifdef DEBUG //_GETTEXTASPOLYGON
    std::cerr << "pos " << aPosition.x << "," << aPosition.y << " displacement "
              << cursorDisplacement.x << "," << cursorDisplacement.y << std::endl;
    {
        std::cerr << aGlyphs.size() << " glyphs; ";
        for( int g = 0; g < (int) aGlyphs.size(); g++ )
        {
            if( g > 0 )
                std::cerr << " ";
            const SHAPE_POLY_SET& theGlyph = aGlyphs.at( g );
            std::cerr << "G" << g << " " << theGlyph.OutlineCount() << " outlines; ";
            for( int i = 0; i < theGlyph.OutlineCount(); i++ )
            {
                std::cerr << "outline " << i << " " << theGlyph.COutline( i ).PointCount()
                          << " points " << theGlyph.HoleCount( i ) << " holes ";
                for( int j = 0; j < theGlyph.HoleCount( i ); j++ )
                    std::cerr << "hole " << i << " " << theGlyph.CHole( i, j ).PointCount()
                              << " points ";
            }
        }
        std::cerr << std::endl;
    }
#endif
    hb_buffer_destroy( buf );

    return VECTOR2I( aPosition.x + cursorDisplacement.x, aPosition.y + cursorDisplacement.y );
}


VECTOR2D OUTLINE_FONT::getBoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                                       TEXT_STYLE_FLAGS aTextStyle ) const
{
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, aString.c_str(), -1, 0, -1 );

    // guess direction, script, and language based on contents
    hb_buffer_guess_segment_properties( buf );

    FT_Face face = mFace;
    int     scaler = mFaceScaler;
    if( IsSubscript( aTextStyle ) || IsSuperscript( aTextStyle ) )
    {
        face = mSubscriptFace;
    }

    hb_font_t* referencedFont = hb_ft_font_create_referenced( face );
    hb_ft_font_set_funcs( referencedFont );
    hb_shape( referencedFont, buf, NULL, 0 );

    unsigned int     glyphCount;
    hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos( buf, &glyphCount );
    //hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( buf, &glyphCount );

    VECTOR2D boundingBox( 0, 0 );

    int    xScaler = aGlyphSize.x / scaler;
    int    yScaler = aGlyphSize.y / scaler;
    double maxHeight = 0.0;
    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        //hb_glyph_position_t& pos = glyphPos[i];
        int codepoint = glyphInfo[i].codepoint;

        FT_Load_Glyph( face, codepoint, FT_LOAD_NO_BITMAP );

        FT_GlyphSlot glyphSlot = face->glyph;
        FT_Glyph     glyph;
        FT_BBox      controlBox;

        FT_Get_Glyph( glyphSlot, &glyph );
        FT_Glyph_Get_CBox( glyph, FT_Glyph_BBox_Mode::FT_GLYPH_BBOX_UNSCALED, &controlBox );

        double width = controlBox.xMax * xScaler;
        boundingBox.x += width;

        double height = controlBox.yMax * yScaler;
        if( height > maxHeight )
            maxHeight = height;

#ifdef DEBUG
        std::cerr << "#" << i << "/" << glyphCount << " codepoint " << codepoint << " boundingBox "
                  << boundingBox << " controlBox " << controlBox.xMax << "," << controlBox.yMax
                  << " aGlyphSize " << aGlyphSize << " scaler " << scaler << " width " << width
                  << " height " << height << " maxHeight " << maxHeight << std::endl;
#endif
        FT_Done_Glyph( glyph );
    }
    boundingBox.y = aGlyphSize.y; //maxHeight;

    hb_buffer_destroy( buf );

#ifdef DEBUG
    std::cerr << "OUTLINE_FONT::getBoundingBox( \"" << aString << "\", " << aGlyphSize << ", "
              << aTextStyle << " ) returns " << boundingBox << std::endl;
#endif

    return boundingBox;
}


#ifdef FOF
void OUTLINE_FONT::RenderToOpenGLCanvas( KIGFX::OPENGL_GAL& aGal, const UTF8& aString,
                                         const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                                         const EDA_ANGLE& aOrientation, bool aIsMirrored ) const
{
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, aString.c_str(), -1, 0, -1 );

    // guess direction, script, and language based on contents
    hb_buffer_guess_segment_properties( buf );

    unsigned int         glyphCount;
    hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( buf, &glyphCount );
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( buf, &glyphCount );
    hb_font_t*           referencedFont = hb_ft_font_create_referenced( mFace );

    hb_ft_font_set_funcs( referencedFont );
    hb_shape( referencedFont, buf, NULL, 0 );

    const double mirror_factor = ( aIsMirrored ? 1 : -1 );
    const double x_scaleFactor = mirror_factor * aGlyphSize.x / mScaler;
    const double y_scaleFactor = aGlyphSize.y / mScaler;

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;

    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        hb_glyph_position_t& pos = glyphPos[i];
        int                  codepoint = glyphInfo[i].codepoint;

        FT_Error e = FT_Load_Glyph( mFace, codepoint, FT_LOAD_DEFAULT );
        // TODO handle FT_Load_Glyph error

        FT_Glyph glyph;
        e = FT_Get_Glyph( mFace->glyph, &glyph );
        // TODO handle FT_Get_Glyph error

        wxPoint pt( aPosition );
        pt.x += ( cursor_x >> 6 ) * x_scaleFactor;
        pt.y += ( cursor_y >> 6 ) * y_scaleFactor;

#if 0
        aGal.DrawGlyph();
#endif

        cursor_x += pos.x_advance;
        cursor_y += pos.y_advance;
    }

    hb_buffer_destroy( buf );
}
#endif
