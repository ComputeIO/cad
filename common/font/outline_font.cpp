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
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <harfbuzz/hb-ft.h>
#include <bezier_curves.h>
#include <geometry/shape_poly_set.h>
#include <eda_text.h>
#include <font/outline_font.h>
#include FT_GLYPH_H
#include FT_BBOX_H
#include <trigo.h>
#include <font/fontconfig.h>

using namespace KIFONT;

FT_Library OUTLINE_FONT::mFreeType = nullptr;

OUTLINE_FONT::OUTLINE_FONT() : mFaceSize( 16 ), mSubscriptSize( 13 )
{
    if( !mFreeType )
    {
        //FT_Error ft_error = FT_Init_FreeType( &mFreeType );
        // TODO: handle ft_error
        FT_Init_FreeType( &mFreeType );
    }
}


bool OUTLINE_FONT::LoadFont( const wxString& aFontName, bool aBold, bool aItalic )
{
    wxString fontFile;
    wxString fontName = getFontNameForFontconfig( aFontName, aBold, aItalic );

    bool r = Fontconfig().FindFont( fontName, fontFile );
    if( r )
    {
        FT_Error e = loadFace( fontFile );
        if( e )
        {
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

    return e;
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
    hb_shape( referencedFont, buf, nullptr, 0 );

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

    return VECTOR2D( width * mFaceScaler, height * mFaceScaler );
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
double OUTLINE_FONT::GetInterline( double aGlyphHeight, double aLineSpacing ) const
{
    //return GetFace()->height;
    //return ( GetFace()->size->metrics.ascender - GetFace()->size->metrics.descender ) >> 6;
    return ( aLineSpacing * aGlyphHeight * ( GetFace()->height / GetFace()->units_per_EM ) );
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


BOX2I OUTLINE_FONT::getBoundingBox( const GLYPH_LIST& aGlyphs ) const
{
    int minX = INT_MAX;
    int minY = INT_MAX;
    int maxX = INT_MIN;
    int maxY = INT_MIN;

    for( auto glyph : aGlyphs )
    {
        BOX2D bbox = glyph->BoundingBox();
        bbox.Normalize();
        if( minX > bbox.GetX() )
            minX = bbox.GetX();
        if( minY > bbox.GetY() )
            minY = bbox.GetY();
        if( maxX < bbox.GetRight() )
            maxX = bbox.GetRight();
        if( maxY < bbox.GetBottom() )
            maxY = bbox.GetBottom();
    }
    BOX2I ret;
    ret.SetOrigin( minX, minY );
    ret.SetEnd( maxX, maxY );
    return ret;
}


VECTOR2I OUTLINE_FONT::GetLinesAsPolygon( GLYPH_LIST& aGlyphs, const EDA_TEXT* aText ) const
{
    wxArrayString         strings;
    std::vector<wxPoint>  positions;
    int                   n;
    VECTOR2I              ret;
    std::vector<VECTOR2D> boundingBoxes;
    TEXT_ATTRIBUTES       attributes( aText );

    getLinePositions( aText->GetShownText(), aText->GetTextPos(), strings, positions, n,
                      boundingBoxes, aText->GetTextSize(), attributes );

    for( int i = 0; i < n; i++ )
    {
        MARKUP::MARKUP_PARSER markupParser( UTF8( strings.Item( i ) ) );
        auto                  markupRoot = markupParser.Parse();
        GLYPH_LIST            lineGlyphs;

        ret = drawMarkup( lineGlyphs, markupRoot, positions[i], aText->GetTextSize(),
                          attributes.GetAngle(), aText->IsMirrored() );

        for( auto glyph : lineGlyphs )
        {
            aGlyphs.push_back( glyph );
        }
    }
    return ret;
}


VECTOR2I OUTLINE_FONT::GetTextAsPolygon( BOX2I* aBoundingBox, GLYPH_LIST& aGlyphs, const UTF8& aText,
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
    hb_shape( referencedFont, buf, nullptr, 0 );

    const double   mirror_factor = ( aIsMirrored ? 1 : -1 );
    const VECTOR2D scaleFactor( glyphSize.x * mirror_factor / scaler, glyphSize.y / scaler );

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

        FT_GlyphSlot faceGlyph = face->glyph;

        // contours is a collection of all outlines in the glyph;
        // example: glyph for 'o' generally contains 2 contours,
        // one for the glyph outline and one for the hole
        CONTOURS contours;

        OUTLINE_DECOMPOSER decomposer( faceGlyph->outline );
        decomposer.OutlineToSegments( &contours );

        SHAPE_POLY_SET                poly;
        std::vector<SHAPE_LINE_CHAIN> holes;
        std::vector<SHAPE_LINE_CHAIN> outlines;

        for( CONTOUR& c : contours )
        {
            GLYPH_POINTS     points = c.points;
            SHAPE_LINE_CHAIN shape;

            VECTOR2D offset( aPosition );

            if( IsSubscript( aTextStyle ) )
                offset.y += glyphSize.y * 0.1;
            else if( IsSuperscript( aTextStyle ) )
                offset.y -= glyphSize.y * 0.2;

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

                //VECTOR2D pt( -v.x / scaler, -v.y / scaler );
                VECTOR2D pt( v.x, v.y );
                VECTOR2D ptC( pt.x + cursor.x, pt.y + cursor.y );
                wxPoint  scaledPtOrig( -ptC.x * scaleFactor.x, -ptC.y * scaleFactor.y );
                wxPoint  scaledPt( scaledPtOrig );
                RotatePoint( &scaledPt, aOrientation.AsRadians() );
                scaledPt.x += offset.x;
                scaledPt.y += offset.y;

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
            }

            if( contourIsHole( c ) )
            {
                holes.push_back( std::move( shape ) );
            }
            else
            {
                outlines.push_back( std::move( shape ) );
            }
        }

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
            }
            nthHole++;
        }

        auto glyph = std::make_shared<OUTLINE_GLYPH>( poly );
        aGlyphs.push_back( glyph );

        cursor.x += pos.x_advance;
        cursor.y += pos.y_advance;
    }

    VECTOR2I cursorEnd( cursor );

    if( IsOverbar( aTextStyle ) )
    {
        SHAPE_POLY_SET   overbarGlyph;
        SHAPE_LINE_CHAIN overbar;

        int left = extentBottomLeft.x;
        int right = extentTopRight.x;
        int top = extentBottomLeft.y - 800;
        int barHeight = -3200;

        overbar.Append( VECTOR2D( left, top ) );
        overbar.Append( VECTOR2D( right, top ) );
        overbar.Append( VECTOR2D( right, top + barHeight ) );
        overbar.Append( VECTOR2D( left, top + barHeight ) );
        overbar.SetClosed( true );

        overbarGlyph.AddOutline( overbar );

        aGlyphs.push_back( std::make_shared<OUTLINE_GLYPH>( overbarGlyph ) );
    }

    hb_buffer_destroy( buf );

    VECTOR2I cursorDisplacement( -cursorEnd.x * scaleFactor.x, cursorEnd.y * scaleFactor.y );

    if( aBoundingBox )
    {
        aBoundingBox->SetOrigin( aPosition.x, aPosition.y );
        aBoundingBox->SetEnd( cursorDisplacement );
    }

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
    hb_shape( referencedFont, buf, nullptr, 0 );

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

        FT_Done_Glyph( glyph );
    }
    boundingBox.y = aGlyphSize.y; //maxHeight;

    hb_buffer_destroy( buf );

    return boundingBox;
}


#undef OUTLINEFONT_RENDER_AS_PIXELS
#ifdef OUTLINEFONT_RENDER_AS_PIXELS
/* WIP: eeschema (and PDF output?) should use pixel rendering instead of
 * linear segmentation
 */
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
    hb_shape( referencedFont, buf, nullptr, 0 );

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

        cursor_x += pos.x_advance;
        cursor_y += pos.y_advance;
    }

    hb_buffer_destroy( buf );
}
#endif //OUTLINEFONT_RENDER_AS_PIXELS
