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

FT_Library OUTLINE_FONT::mFreeType = nullptr;

OUTLINE_FONT::OUTLINE_FONT()
{
    if( !mFreeType )
    {
        //FT_Error ft_error = FT_Init_FreeType( &mFreeType );
        // TODO: handle ft_error
        FT_Init_FreeType( &mFreeType );
    }
}


bool OUTLINE_FONT::LoadFont( const wxString& aFontFileName )
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
    // TODO: check that going from wxString to char* with UTF-8
    // conversion for filename makes sense on any/all platforms
    return FT_New_Face( mFreeType, aFontFileName.mb_str( wxConvUTF8 ), 0, &mFace );
}


/**
 * Draw a string.
 *
 * @param aGal
 * @param aText is the text to be drawn.
 * @param aPosition is the text position in world coordinates.
 * @param aRotationAngle is the text rotation angle in radians.
 */
void OUTLINE_FONT::Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                         double aRotationAngle ) const
{
#ifdef DEBUG
    if( debugMe( aText ) )
        std::cerr << "OUTLINE_FONT::Draw( aGal, \"" << aText << "\", " << aPosition << ", "
                  << aRotationAngle << " ) const\n";
#endif
    if( aText.empty() )
        return;

    FT_Select_Charmap( mFace, FT_Encoding::FT_ENCODING_UNICODE );
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
    FT_Set_Char_Size( mFace, 0, 16 * 64, 0, 0 );

    // Context needs to be saved before any transformations
    aGal->Save();
    aGal->Translate( aPosition );
    aGal->Rotate( -aRotationAngle );

    wxPoint pt( 0, 0 );
    drawSingleLineText( aGal, aText, pt, 0.0 );

    aGal->Restore();
}


/**
 * Compute the boundary limits of aText (the bounding box of all shapes).
 *
 * The overbar and alignment are not taken in account, '~' characters are skipped.
 *
 * @return a VECTOR2D giving the width and height of text.
 */
VECTOR2D OUTLINE_FONT::ComputeStringBoundaryLimits( const KIGFX::GAL* aGal, const UTF8& aText,
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

    return VECTOR2D( width, height );
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
    assert( 1 == 0 ); // TODO!!!
}


/**
 * Compute the distance (interline) between 2 lines of text (for multiline texts).
 *
 * @param aGlyphHeight is the height (vertical size) of the text.
 * @return the interline.
 */
double OUTLINE_FONT::GetInterline( double aGlyphHeight ) const
{
    static const double interlinePitchRatio = 1.61; //INTERLINE_PITCH_RATIO in stroke_font.cpp
    const int lineHeight = ( mFace->size->metrics.ascender - mFace->size->metrics.descender ) >> 6;

    double ret = aGlyphHeight * interlinePitchRatio;
    //lineHeight; // aGlyphHeight * lineHeight; // * mScaler;
#ifdef FOOBAR //DEBUG
    std::cerr << "OUTLINE_FONT::GetInterline( " << aGlyphHeight << " ) const returns " << ret
              << " (lineHeight " << lineHeight << ")" << std::endl;
#endif
    return ret;
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
    assert( 1 == 0 ); // TODO!!!
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


void OUTLINE_FONT::drawSingleLineText( KIGFX::GAL* aGal, const UTF8& aText,
                                       const VECTOR2D& aPosition, double aAngle ) const
{
#ifdef DEBUG
    if( debugMe( aText ) )
        std::cerr << "OUTLINE_FONT::drawSingleLineText( aGal, \"" << aText << "\", " << aPosition
                  << " )";
#endif
#if 0
    if( aGal->IsOpenGlEngine() )
    {
        // draw with OpenGL routines to get antialiased pixmaps
        // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //aGal->GetFreeType()->Render( this, aText );
    }
    // for now let's just do what we do for all GAL types
#endif
    // transform glyph outlines to straight segments, then fill
    std::vector<SHAPE_POLY_SET> glyphs;
    wxPoint                     pt( aPosition.x, aPosition.y );
    GetTextAsPolygon( glyphs, aText, aGal->GetGlyphSize(), pt, aAngle, aGal->IsTextMirrored() );

    //aGal->SetIsFill( true );
    aGal->DrawGlyphs( glyphs );
}


static inline int getp2( int a )
{
    int rval = 1;
    while( rval < a )
        rval <<= 1;
    return rval;
}


void OUTLINE_FONT::GetTextAsPolygon( std::vector<SHAPE_POLY_SET>& aGlyphs, const UTF8& aText,
                                     const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                                     double aOrientation, bool aIsMirrored ) const
{
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, aText.c_str(), -1, 0, -1 );

    // guess direction, script, and language based on contents
    hb_buffer_guess_segment_properties( buf );

    unsigned int         glyphCount;
    hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( buf, &glyphCount );
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( buf, &glyphCount );
    hb_font_t*           referencedFont = hb_ft_font_create_referenced( mFace );

    hb_ft_font_set_funcs( referencedFont );
    hb_shape( referencedFont, buf, NULL, 0 );

    const double mirror_factor = ( aIsMirrored ? 1 : -1 );
    const double x_scale_factor = mirror_factor * aGlyphSize.x / mScaler;
    const double y_scale_factor = aGlyphSize.y / mScaler;
    const double advance_scale_factor = 1;
    const double advance_x_factor = advance_scale_factor;
    const double advance_y_factor = advance_scale_factor;

#ifdef DEBUG //STROKEFONT
    if( debugMe( aText ) )
    {
        std::cerr << "[OUTLINE_FONT::GetTextAsPolygon( &aGlyphs, \"" << aText << "\", "
                  << aGlyphSize << ", " << aPosition << ", " << aOrientation << ", "
                  << ( aIsMirrored ? "true" : "false" ) << " ) const; glyphCount " << glyphCount
                  << " mirror_factor " << mirror_factor << " mScaler " << mScaler
                  << " scale_factor x" << x_scale_factor << " y" << y_scale_factor << " font "
                  << Name() << " c_str " << aText.c_str() << " ";

        for( unsigned int i = 0; i < glyphCount; i++ )
        {
            hb_glyph_position_t& pos = glyphPos[i];
            int                  codepoint = glyphInfo[i].codepoint;

            FT_Load_Glyph( mFace, codepoint, FT_LOAD_NO_BITMAP );

            FT_GlyphSlot              glyph = mFace->glyph;
            static const unsigned int bufsize = 512;
            char                      glyphName[bufsize];
            FT_Get_Glyph_Name( mFace, glyph->glyph_index, &glyphName[0], bufsize );
            std::cerr << "<glyph " << codepoint << " '" << glyphName << "' pos ";
            std::cerr << pos.x_advance << "," << pos.y_advance << "," << pos.x_offset << ","
                      << pos.y_offset << ">";
        }
        std::cerr << "]" << std::endl;
    }
#endif

#if 0
    hb_position_t cursor_x = aPosition.x;
    hb_position_t cursor_y = aPosition.y;
#else
    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;
#endif

    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        hb_glyph_position_t& pos = glyphPos[i];
        int                  codepoint = glyphInfo[i].codepoint;

        FT_Load_Glyph( mFace, codepoint, FT_LOAD_NO_BITMAP );

        FT_GlyphSlot glyph = mFace->glyph;

        // contours is a collection of all outlines in the glyph;
        // example: glyph for 'o' generally contains 2 contours,
        // one for the glyph outline and one for the hole
        CONTOURS contours;

        OUTLINE_DECOMPOSER decomposer( glyph->outline );
        decomposer.OutlineToSegments( &contours );
#ifdef DEBUG
        static const unsigned int bufsize = 512;
        char                      glyphName[bufsize];
        FT_Get_Glyph_Name( mFace, glyph->glyph_index, &glyphName[0], bufsize );
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

        VECTOR2D offset( aPosition );

        for( CONTOUR& c : contours )
        {
            POINTS points = c.points;
#ifdef DEBUG
            int ptCount = 0;
            std::cerr << "c.points.size()==" << c.points.size() << " ";
#endif
            SHAPE_LINE_CHAIN shape;

            for( const VECTOR2D& v : points )
            {
                VECTOR2D pt( -( v.x + cursor_x ), -( v.y + cursor_y ) );
#if 0
                VECTOR2D scaledPt( pt );
#else
                wxPoint scaledPt( pt.x * x_scale_factor, pt.y * y_scale_factor );
                RotatePoint( &scaledPt, aOrientation );
                scaledPt.x += offset.x;
                scaledPt.y += offset.y;
#endif
#ifdef FOOBAR //DEBUG
                std::cerr << "(p#" << ptCount << " " << scaledPt;
#endif
                shape.Append( scaledPt.x, scaledPt.y );
                //ptListScaled.push_back( scaledPt );
#ifdef FOOBAR //DEBUG
                std::cerr << "->n " << shape.PointCount() << ")";
#endif
#ifdef DEBUG
                ptCount++;
#endif
            }

#ifdef DEBUG
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

#ifdef DEBUG
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
#ifdef DEBUG
                else
                {
                    std::cerr << "Could not find outline for hole " << nthHole << "! ";
                }
#endif
            }
            nthHole++;
        }

        aGlyphs.push_back( poly );

        cursor_x += ( pos.x_advance * advance_x_factor );
        cursor_y += ( pos.y_advance * advance_y_factor );
    }
#ifdef DEBUG
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
}


#ifdef FOF
void OUTLINE_FONT::RenderToOpenGLCanvas( KIGFX::OPENGL_GAL& aGal, const UTF8& aString,
                                         const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                                         double aOrientation, bool aIsMirrored ) const
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
    const double x_scale_factor = mirror_factor * aGlyphSize.x / mScaler;
    const double y_scale_factor = aGlyphSize.y / mScaler;
    const double advance_scale_factor = 1;
    const double advance_x_factor = advance_scale_factor;
    const double advance_y_factor = advance_scale_factor;

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
        pt.x += ( cursor_x >> 6 ) * x_scale_factor;
        pt.y += ( cursor_y >> 6 ) * y_scale_factor;

#if 0
        aGal.DrawGlyph();
#endif

        cursor_x += ( pos.x_advance * advance_x_factor );
        cursor_y += ( pos.y_advance * advance_y_factor );
    }

    hb_buffer_destroy( buf );
}
#endif
