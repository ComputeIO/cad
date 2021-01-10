/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2016 Kicad Developers, see change_log.txt for contributors.
 *
 * TrueType font class
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

#include <wx/string.h>
#include <settings/settings_manager.h>
#include <gal/truetype_font.h>
#include FT_GLYPH_H
#include FT_BBOX_H
#include FT_OUTLINE_H
#include <harfbuzz/hb-ft.h>

using namespace KIGFX;

FT_Library TRUETYPE_FONT::mFreeType = nullptr;


TRUETYPE_FONT::TRUETYPE_FONT()
{
    if( !mFreeType )
    {
        //FT_Error ft_error = FT_Init_FreeType( &mFreeType );
        // TODO: handle ft_error
        FT_Init_FreeType( &mFreeType );
    }
}


bool TRUETYPE_FONT::LoadFont( const wxString& aFontFileName )
{
#ifdef DEBUG
    std::cerr << "TRUETYPE_FONT::LoadFont( \"" << aFontFileName << "\" )" << std::endl;
#endif

    wxFileName fontFile( aFontFileName );
    wxString   fileName = fontFile.GetFullPath();
    // TODO: handle ft_error properly (now we just return false if load does not succeed)
    FT_Error ft_error = loadFace( fileName );

    if( ft_error )
    {
        // Try user dir
        fontFile.SetExt( "otf" );
        fontFile.SetPath( SETTINGS_MANAGER::GetUserSettingsPath() + wxT( "/fonts" ) );
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
        m_fontFileName = fileName;
    }

#ifdef DEBUG
    std::cerr << aFontFileName << " loaded from " << fileName << std::endl;
    std::cerr << "Family " << mFace->family_name << " Style " << mFace->style_name << " has "
              << mFace->num_faces << " faces and " << mFace->num_glyphs << " glyphs." << std::endl;
#endif
    return true;
}


FT_Error TRUETYPE_FONT::loadFace( const wxString& aFontFileName )
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
void TRUETYPE_FONT::Draw( GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                          double aRotationAngle ) const
{
    if( aText.empty() )
        return;

    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, aText.c_str(), -1, 0, -1 );

    hb_buffer_set_direction( buf, HB_DIRECTION_LTR );
    hb_buffer_set_script( buf, HB_SCRIPT_LATIN );
    hb_buffer_set_language( buf, hb_language_from_string( "en", -1 ) );

    FT_Select_Charmap( mFace, FT_Encoding::FT_ENCODING_UNICODE );
    FT_Set_Char_Size( mFace, 0, 1000, 0, 0 );

#if 0
    hb_blob_t* blob = hb_blob_create_from_file( m_fontFileName.c_str() );
    hb_face_t* face = hb_face_create( blob, 0 );
    hb_font_t* font = hb_font_create( face );
#else
    hb_font_t* font = hb_ft_font_create_referenced( mFace );
    hb_ft_font_set_funcs( font );
#endif

    hb_shape( font, buf, NULL, 0 );

    unsigned int         glyph_count;
    hb_glyph_info_t*     glyph_info = hb_buffer_get_glyph_infos( buf, &glyph_count );
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions( buf, &glyph_count );

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;
    for( unsigned int i = 0; i < glyph_count; i++ )
    {
        hb_codepoint_t glyphid = glyph_info[i].codepoint;
        hb_position_t  x_offset = glyph_pos[i].x_offset;
        hb_position_t  y_offset = glyph_pos[i].y_offset;
        hb_position_t  x_advance = glyph_pos[i].x_advance;
        hb_position_t  y_advance = glyph_pos[i].y_advance;
        /* draw_glyph(glyphid, cursor_x + x_offset, cursor_y + y_offset); */
        std::cerr << "glyph #" << i << "/" << glyph_count << " id " << glyphid << " x " << cursor_x
                  << "+" << x_advance << " offset " << x_offset << " y " << cursor_y << "+"
                  << y_advance << " offset " << y_offset << std::endl;
        cursor_x += x_advance;
        cursor_y += y_advance;
    }

    // Context needs to be saved before any transformations
    aGal->Save();

    aGal->Translate( aPosition );
    aGal->Rotate( -aRotationAngle );
    const VECTOR2D& glyphSize = aGal->GetGlyphSize();

    std::cerr << "GAL glyphSize == " << glyphSize << std::endl;

#ifdef FOOFAA
    // Single line height
    int lineHeight = KiROUND( GetInterline( m_gal->GetGlyphSize().y ) );
    int lineCount = linesCount( aText );
    //const VECTOR2D& glyphSize = m_gal->GetGlyphSize();

    // align the 1st line of text
    switch( m_gal->GetVerticalJustify() )
    {
    case GR_TEXT_VJUSTIFY_TOP: m_gal->Translate( VECTOR2D( 0, glyphSize.y ) ); break;

    case GR_TEXT_VJUSTIFY_CENTER: m_gal->Translate( VECTOR2D( 0, glyphSize.y / 2.0 ) ); break;

    case GR_TEXT_VJUSTIFY_BOTTOM: break;

    default: break;
    }

    if( lineCount > 1 )
    {
        switch( m_gal->GetVerticalJustify() )
        {
        case GR_TEXT_VJUSTIFY_TOP: break;

        case GR_TEXT_VJUSTIFY_CENTER:
            m_gal->Translate( VECTOR2D( 0, -( lineCount - 1 ) * lineHeight / 2 ) );
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            m_gal->Translate( VECTOR2D( 0, -( lineCount - 1 ) * lineHeight ) );
            break;
        }
    }

    m_gal->SetIsStroke( true );
    //m_gal->SetIsFill( false );

    if( m_gal->IsFontBold() )
    {
        // TODO: figure out how to do real bold
        m_gal->SetLineWidth( m_gal->GetLineWidth() * 1.3 );
    }

    // Split multiline strings into separate ones and draw them line by line
    size_t begin = 0;
    size_t newlinePos = aText.find( '\n' );

    while( newlinePos != aText.npos )
    {
        size_t length = newlinePos - begin;

        drawSingleLineText( aText.substr( begin, length ) );
        m_gal->Translate( VECTOR2D( 0.0, lineHeight ) );

        begin = newlinePos + 1;
        newlinePos = aText.find( '\n', begin );
    }

    // Draw the last (or the only one) line
    if( !aText.empty() )
        drawSingleLineText( aText.substr( begin ) );

#endif // FOOFAA

    drawSingleLineText( aGal, buf, font );

    hb_buffer_destroy( buf );
#if 0
    hb_font_destroy( font );
    hb_face_destroy( face );
    hb_blob_destroy( blob );
#endif
    aGal->Restore();
}


/**
 * Compute the boundary limits of aText (the bounding box of all shapes).
 *
 * The overbar and alignment are not taken in account, '~' characters are skipped.
 *
 * @return a VECTOR2D giving the width and height of text.
 */
VECTOR2D TRUETYPE_FONT::ComputeStringBoundaryLimits( const GAL* aGal, const UTF8& aText,
                                                     const VECTOR2D& aGlyphSize,
                                                     double          aGlyphThickness ) const
{
    assert( 1 == 0 ); // TODO!!!
}


/**
 * Compute the vertical position of an overbar, sometimes used in texts.
 *
 * This is the distance between the text base line and the overbar.
 *
 * @param aGlyphHeight is the height (vertical size) of the text.
 * @return the relative position of the overbar axis.
 */
double TRUETYPE_FONT::ComputeOverbarVerticalPosition( double aGlyphHeight ) const
{
    assert( 1 == 0 ); // TODO!!!
}


/**
 * Compute the distance (interline) between 2 lines of text (for multiline texts).
 *
 * @param aGlyphHeight is the height (vertical size) of the text.
 * @return the interline.
 */
double TRUETYPE_FONT::GetInterline( double aGlyphHeight ) const
{
    assert( 1 == 0 ); // TODO!!!
}


/**
 * Compute the X and Y size of a given text. The text is expected to be
 * a only one line text.
 *
 * @param aText is the text string (one line).
 * @return the text size.
 */
VECTOR2D TRUETYPE_FONT::ComputeTextLineSize( const GAL* aGal, const UTF8& aText ) const
{
    assert( 1 == 0 ); // TODO!!!
}


void TRUETYPE_FONT::drawSingleLineText( GAL* aGal, hb_buffer_t* aText, hb_font_t* aFont ) const
{
    const VECTOR2D& galGlyphSize = aGal->GetGlyphSize();

    // Context needs to be saved before any transformations
    aGal->Save();

    unsigned int         glyphCount;
    hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( aText, &glyphCount );
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( aText, &glyphCount );

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;
    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        hb_codepoint_t glyphid = glyphInfo[i].codepoint;
        hb_position_t  x_offset = glyphPos[i].x_offset;
        hb_position_t  y_offset = glyphPos[i].y_offset;
        hb_position_t  x_advance = glyphPos[i].x_advance;
        hb_position_t  y_advance = glyphPos[i].y_advance;
        /* draw_glyph(glyphid, cursor_x + x_offset, cursor_y + y_offset); */
        std::cerr << "Glyph #" << i << "/" << glyphCount << " id " << glyphid << " x " << cursor_x
                  << "+" << x_advance << " offset " << x_offset << " y " << cursor_y << "+"
                  << y_advance << " offset " << y_offset << std::endl;
        cursor_x += x_advance;
        cursor_y += y_advance;
    }

    const int             GLYPH_NAME_LEN = 256;
    char                  glyph_name[GLYPH_NAME_LEN];
    std::vector<VECTOR2D> ptListScaled;

    const double mirror_factor = ( aGal->IsTextMirrored() ? 1 : -1 );
    const double xyscaler = 3.0e2;
    const double x_scale_factor = mirror_factor * galGlyphSize.x / xyscaler;
    const double y_scale_factor = galGlyphSize.y / xyscaler;
    const double advance_scale_factor = 1.5; // / 10.0;

    cursor_x = 0;
    cursor_y = 0;
    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        int                  cluster = glyphInfo[i].cluster;
        int                  codepoint = glyphInfo[i].codepoint;
        hb_glyph_position_t& pos = glyphPos[i];
        std::cerr << "ch[" << i << "] cluster " << cluster << " codepoint " << codepoint
                  << " advance " << pos.x_advance << "," << pos.y_advance << " offset "
                  << pos.x_offset << "," << pos.y_offset << std::endl;

        //FT_Load_Char( mFace, codepoint, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE );
        FT_Load_Glyph( mFace, codepoint, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE );
        FT_Get_Glyph_Name( mFace, codepoint, glyph_name, GLYPH_NAME_LEN );

        FT_GlyphSlot glyph = mFace->glyph;
        unsigned int glyph_index = glyph->glyph_index;

        std::cerr << "glyph name [" << glyph_name << "] " << glyph_index << std::endl;

        FT_Outline outline = glyph->outline;

        int ptCount = 0;
        ptListScaled.clear();

#ifdef FOO
        for( unsigned int p = 0; p < (unsigned int) outline.n_points; p++ )
        {
            FT_Vector    point = outline.points[p];
            char         tags = outline.tags[p];
            unsigned int onCurve = ( tags & 0x1 );
            unsigned int thirdOrderBezierPoint = ( onCurve ? ( tags & 0x2 ) : 0 );
            unsigned int hasDropout = ( tags & 0x4 );
            unsigned int dropoutMode = ( hasDropout ? ( tags & ( 0x8 | 0x10 | 0x20 ) ) : 0 );

#ifdef TTF_GAL_TRANSFORM
            VECTOR2D pt( point.x + cursor_x, point.y + cursor_y );
            VECTOR2D scaledPt( pt.x * x_scale_factor, pt.y * y_scale_factor );
#else
            // TODO: figure out how to use aGal->Transform( ... ) instead
            // of hacking the coordinates
            VECTOR2D pt( -( point.x + cursor_x ), -( point.y + cursor_y ) );
            VECTOR2D scaledPt( pt.x * x_scale_factor, pt.y * y_scale_factor );
#endif

            std::cerr << "Point [" << scaledPt.x << "," << scaledPt.y << "] "
                      << ( onCurve ? "on curve " : "off curve " )
                      << ( onCurve ? ""
                                   : ( thirdOrderBezierPoint ? "3rd order Bezier point, "
                                                             : "2nd order Bezier point, " ) )
                      << ( hasDropout ? "dropout " : "no dropout " )
                      << ( hasDropout ? dropoutMode : 0 ) << std::endl;

            ptListScaled.push_back( scaledPt );
            ptCount++;
        }
#else
        std::vector<VECTOR2D> straightSegments = outlineToStraightSegments( outline );
        for( VECTOR2D v : straightSegments)
        {
            VECTOR2D pt( -( v.x + cursor_x ), -( v.y + cursor_y ) );
            VECTOR2D scaledPt( pt.x * x_scale_factor, pt.y * y_scale_factor );
            ptListScaled.push_back( scaledPt );
            ptCount++;
        }
#endif // FOO

        if( !ptListScaled.empty() )
        {
            aGal->DrawPolyline( &ptListScaled[0], ptCount );
        }

        cursor_x += ( pos.x_advance * advance_scale_factor );
        cursor_y += ( pos.y_advance * advance_scale_factor );
    }

    // Restore context
    aGal->Restore();
}


std::vector<VECTOR2D> TRUETYPE_FONT::outlineToStraightSegments( FT_Outline aOutline ) const
{
    std::vector<VECTOR2D> segments;
    std::vector<VECTOR2D> bezier;

    FT_Vector    prev_point;
    FT_Vector    prev_prev_point;
    unsigned int bezierControlPointCount = 0;
    for( unsigned int p = 0; p < (unsigned int) aOutline.n_points; p++ )
    {
        FT_Vector    point = aOutline.points[p];
        char         tags = aOutline.tags[p];
        unsigned int onCurve = ( tags & 0x1 );
        unsigned int thirdOrderBezierPoint = ( onCurve ? ( tags & 0x2 ) : 0 );
        unsigned int hasDropout = ( tags & 0x4 );
        unsigned int dropoutMode = ( hasDropout ? ( tags & ( 0x8 | 0x10 | 0x20 ) ) : 0 );

#ifdef DEBUG
        std::cerr << "Point [" << point.x << "," << point.y << "] "
                  << ( onCurve ? "on curve " : "off curve " )
                  << ( onCurve ? ""
                               : ( thirdOrderBezierPoint ? "3rd order Bezier point"
                                                         : "2nd order Bezier point" ) );
        if( hasDropout )
        {
            std::cerr << ", dropout " << dropoutMode;
        }
        std::cerr << std::endl;
#endif

        if( onCurve )
        {
            // straight segment
            VECTOR2D pt( point.x, point.y );
            segments.push_back( pt );
        }
        else
        {
            // Bezier control point
            if( thirdOrderBezierPoint )
            {
                // TODO
                assert( 1 == 0 );
            }
            else
            {
                if( bezierControlPointCount == 0 )
                {
                    VECTOR2D bp( prev_point.x, prev_point.y );
                    bezier.push_back( bp );
                }
                VECTOR2D bp( point.x, point.y );
                bezier.push_back( bp );
                bezierControlPointCount++;
                if( bezierControlPointCount == 2 )
                {
                    std::vector<VECTOR2D> approximation = approximateBezierCurve( bezier );
                    segments.insert( segments.end(), approximation.begin(), approximation.end() );
                    bezierControlPointCount = 0;
                    bezier.clear();
                }
            }
        }

        // TODO: use a stack for these
        prev_prev_point = prev_point;
        prev_point = point;
    }

    return segments;
}

VECTOR2D TRUETYPE_FONT::approximate( const double t, const double oneMinusT, const VECTOR2D& pt1,
                                     const VECTOR2D& pt2 ) const
{
    VECTOR2D pt1prime( pt1.x * oneMinusT, pt1.y * oneMinusT );
    VECTOR2D pt2prime( pt2.x * t, pt2.y * t );
    VECTOR2D r = pt1prime + pt2prime;
    return r;
}

std::vector<VECTOR2D> TRUETYPE_FONT::approximateBezierCurve( std::vector<VECTOR2D> bezier ) const
{
    std::vector<VECTOR2D> approximatedPoints;
    std::vector<VECTOR2D> resultPoints;

    const double precision = 0.1;
    for( double t = 0; t < 1; t += precision )
    {
        double oneMinusT = ( 1 - t );
        approximatedPoints = bezier;
        while( approximatedPoints.size() > 1 )
        {
            auto ptIter = approximatedPoints.begin();
            auto ptIterEnd = std::prev( approximatedPoints.end() );
            while( ptIter != ptIterEnd )
            {
                auto        updateThisIter = ptIter;
                const auto& pt1 = *( ptIter++ );
                const auto& pt2 = *( ptIter );
                const auto  approximatedPoint = approximate( t, oneMinusT, pt1, pt2 );
                *updateThisIter = approximatedPoint;
                approximatedPoints.pop_back();
            }
            resultPoints.push_back( approximatedPoints[0] );
        }
    }
    return resultPoints;
}
