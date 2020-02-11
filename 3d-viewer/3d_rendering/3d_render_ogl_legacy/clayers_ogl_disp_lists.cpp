/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "clayers_ogl_disp_lists.h"

#include "geom3d/clayer_triangles.h"

CLAYERS_OGL_DISP_LISTS::CLAYERS_OGL_DISP_LISTS( const CLAYER_TRIANGLES &aLayerTriangles,
                                                GLuint aTextureIndexForSegEnds,
                                                float aZBot,
                                                float aZTop )
{
    m_zBot = aZBot;
    m_zTop = aZTop;

    m_layer_top_segment_ends        = 0;
    m_layer_top_triangles           = 0;
    m_layer_middle_contourns_quads  = 0;
    m_layer_bot_triangles           = 0;
    m_layer_bot_segment_ends        = 0;

    if( aTextureIndexForSegEnds )
    {
        wxASSERT( glIsTexture( aTextureIndexForSegEnds ) );

        if( glIsTexture( aTextureIndexForSegEnds ) )
        {
            m_layer_top_segment_ends =
                    generate_top_or_bot_seg_ends( aLayerTriangles.m_layer_top_segment_ends,
                                                  true,
                                                  aTextureIndexForSegEnds );

            m_layer_bot_segment_ends =
                    generate_top_or_bot_seg_ends( aLayerTriangles.m_layer_bot_segment_ends,
                                                  false,
                                                  aTextureIndexForSegEnds );
        }
    }

    m_layer_top_triangles = generate_top_or_bot_triangles( aLayerTriangles.m_layer_top_triangles,
                                                           true );

    m_layer_bot_triangles = generate_top_or_bot_triangles( aLayerTriangles.m_layer_bot_triangles,
                                                           false );


    if( aLayerTriangles.m_layer_middle_contourns_quads->GetVertexSize() > 0 )
    {
        m_layer_middle_contourns_quads =
                generate_middle_triangles( aLayerTriangles.m_layer_middle_contourns_quads );
    }

    m_draw_it_transparent = false;
    m_haveTransformation  = false;
    m_zPositionTransformation = 0.0f;
    m_zScaleTransformation    = 0.0f;
}


CLAYERS_OGL_DISP_LISTS::~CLAYERS_OGL_DISP_LISTS()
{
    if( glIsList( m_layer_top_segment_ends ) )
        glDeleteLists( m_layer_top_segment_ends, 1 );

    if( glIsList( m_layer_top_triangles ) )
        glDeleteLists( m_layer_top_triangles, 1 );

    if( glIsList( m_layer_middle_contourns_quads ) )
        glDeleteLists( m_layer_middle_contourns_quads, 1 );

    if( glIsList( m_layer_bot_triangles ) )
        glDeleteLists( m_layer_bot_triangles, 1 );

    if( glIsList( m_layer_bot_segment_ends ) )
        glDeleteLists( m_layer_bot_segment_ends, 1 );

    m_layer_top_segment_ends        = 0;
    m_layer_top_triangles           = 0;
    m_layer_middle_contourns_quads  = 0;
    m_layer_bot_triangles           = 0;
    m_layer_bot_segment_ends        = 0;
}


void CLAYERS_OGL_DISP_LISTS::DrawTopAndMiddle() const
{
    beginTransformation();

    if( glIsList( m_layer_middle_contourns_quads ) )
        glCallList( m_layer_middle_contourns_quads );

    if( glIsList( m_layer_top_triangles ) )
        glCallList( m_layer_top_triangles );

    if( glIsList( m_layer_top_segment_ends ) )
        glCallList( m_layer_top_segment_ends );

    endTransformation();
}


void CLAYERS_OGL_DISP_LISTS::DrawBotAndMiddle() const
{
    beginTransformation();

    if( glIsList( m_layer_middle_contourns_quads ) )
        glCallList( m_layer_middle_contourns_quads );

    if( glIsList( m_layer_bot_triangles ) )
        glCallList( m_layer_bot_triangles );

    if( glIsList( m_layer_bot_segment_ends ) )
        glCallList( m_layer_bot_segment_ends );

    endTransformation();
}


void CLAYERS_OGL_DISP_LISTS::DrawTop() const
{
    beginTransformation();

    if( glIsList( m_layer_top_triangles ) )
        glCallList( m_layer_top_triangles );

    if( glIsList( m_layer_top_segment_ends ) )
        glCallList( m_layer_top_segment_ends );

    endTransformation();
}


void CLAYERS_OGL_DISP_LISTS::DrawBot() const
{
    beginTransformation();

    if( glIsList( m_layer_bot_triangles ) )
        glCallList( m_layer_bot_triangles );

    if( glIsList( m_layer_bot_segment_ends ) )
        glCallList( m_layer_bot_segment_ends );

    endTransformation();
}


void CLAYERS_OGL_DISP_LISTS::DrawMiddle() const
{
    beginTransformation();

    if( glIsList( m_layer_middle_contourns_quads ) )
        glCallList( m_layer_middle_contourns_quads );

    endTransformation();
}


void CLAYERS_OGL_DISP_LISTS::DrawAll( bool aDrawMiddle ) const
{
    beginTransformation();

    if( aDrawMiddle )
        if( glIsList( m_layer_middle_contourns_quads ) )
            glCallList( m_layer_middle_contourns_quads );

    if( glIsList( m_layer_top_triangles ) )
        glCallList( m_layer_top_triangles );

    if( glIsList( m_layer_bot_triangles ) )
        glCallList( m_layer_bot_triangles );

    if( glIsList( m_layer_top_segment_ends ) )
        glCallList( m_layer_top_segment_ends );

    if( glIsList( m_layer_bot_segment_ends ) )
        glCallList( m_layer_bot_segment_ends );

    endTransformation();
}


void CLAYERS_OGL_DISP_LISTS::DrawAllCameraCulled(float zCameraPos, bool aDrawMiddle ) const
{
    zCameraPos = m_haveTransformation?( (zCameraPos - m_zPositionTransformation ) /
                                        m_zScaleTransformation ):zCameraPos;

    //if( aDrawMiddle )
        DrawMiddle();

    if( zCameraPos > m_zTop )
    {
        DrawTop();
    }
    else
    {
        if( zCameraPos < m_zBot )
        {
            DrawBot();
        }
        else
        {
            // If camera is in the middle dont draw it
        }
    }
}


void CLAYERS_OGL_DISP_LISTS::DrawAllCameraCulledSubtractLayer(
        const CLAYERS_OGL_DISP_LISTS *aLayerToSubtractA,
        const CLAYERS_OGL_DISP_LISTS *aLayerToSubtractB,
        bool aDrawMiddle ) const
{
    //if( aDrawMiddle )
        DrawMiddle();

    glClearStencil( 0x00 );
    glClear( GL_STENCIL_BUFFER_BIT );

    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glDisable( GL_DEPTH_TEST );
    glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
    glDepthMask( GL_FALSE );
    glEnable( GL_STENCIL_TEST );
    glStencilFunc( GL_ALWAYS, 1, 0 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );

    if( aLayerToSubtractA )
        aLayerToSubtractA->DrawBot();

    if( aLayerToSubtractB )
        aLayerToSubtractB->DrawBot();


    //if( !m_draw_it_transparent )
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }

    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glStencilFunc( GL_EQUAL, 0, 1 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
    DrawBot();

    glDisable( GL_DEPTH_TEST );
    glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
    glDepthMask( GL_FALSE );
    glEnable( GL_STENCIL_TEST );
    glStencilFunc( GL_ALWAYS, 2, 0 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );

    if( aLayerToSubtractA )
        aLayerToSubtractA->DrawTop();

    if( aLayerToSubtractB )
        aLayerToSubtractB->DrawTop();

    //if( !m_draw_it_transparent )
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }

    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glStencilFunc( GL_NOTEQUAL, 2, 0x03 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );
    DrawTop();


    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );

    glCullFace( GL_FRONT );
    glStencilFunc( GL_GEQUAL, 3, 0x03 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

    if( aDrawMiddle )
    {
        if( aLayerToSubtractA )
            aLayerToSubtractA->DrawMiddle();

        // It will not render the middle contours of the layer.
        // It is used with vias and holes (copper vias and to subtract solder
        // mask holes). But since in the vias, it will draw a cylinder
        // and in soldermask it doesn't need to draw the contour.
        // so it is not used the middle part of B
//        if( aLayerToSubtractB )
//            aLayerToSubtractB->DrawMiddle();
    }

    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE );

    glCullFace( GL_BACK );
    glDisable( GL_STENCIL_TEST );

/*
    if( m_draw_it_transparent )
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }*/
}


void CLAYERS_OGL_DISP_LISTS::ApplyScalePosition( float aZposition,
                                                 float aZscale )
{
    wxASSERT( aZscale > FLT_EPSILON );

    m_zPositionTransformation = aZposition;
    m_zScaleTransformation = aZscale;
    m_haveTransformation = true;
}


void CLAYERS_OGL_DISP_LISTS::SetItIsTransparent( bool aSetTransparent )
{
    m_draw_it_transparent = aSetTransparent;
}


GLuint CLAYERS_OGL_DISP_LISTS::generate_top_or_bot_seg_ends(
        const CLAYER_TRIANGLE_CONTAINER *aTriangleContainer,
        bool aIsNormalUp,
        GLuint aTextureId ) const
{
    wxASSERT( aTriangleContainer != NULL );

    wxASSERT( (aTriangleContainer->GetVertexSize() % 3) == 0 );

    // Top and Bot dont have normals array stored in container
    wxASSERT( aTriangleContainer->GetNormalsSize() == 0 );

    if( (aTriangleContainer->GetVertexSize() > 0) &&
        ((aTriangleContainer->GetVertexSize() % 3) == 0) )
    {
        GLuint listIdx = glGenLists( 1 );

        if( glIsList( listIdx ) )
        {
            // Prepare an array of UV text coordinates
            SFVEC2F *uvArray = new SFVEC2F[aTriangleContainer->GetVertexSize()];

            for( unsigned int i = 0;
                 i < aTriangleContainer->GetVertexSize();
                 i += 3 )
            {
                uvArray[i + 0] = SFVEC2F( 1.0f, 0.0f );
                uvArray[i + 1] = SFVEC2F( 0.0f, 1.0f );
                uvArray[i + 2] = SFVEC2F( 0.0f, 0.0f );
            }

            glEnableClientState( GL_TEXTURE_COORD_ARRAY );
            glDisableClientState( GL_COLOR_ARRAY );
            glDisableClientState( GL_NORMAL_ARRAY );
            glEnableClientState( GL_VERTEX_ARRAY );
            glVertexPointer( 3, GL_FLOAT, 0, aTriangleContainer->GetVertexPointer() );
            glTexCoordPointer( 2, GL_FLOAT, 0, uvArray );

            glNewList( listIdx, GL_COMPILE );

            glDisable( GL_COLOR_MATERIAL );

            glEnable( GL_TEXTURE_2D );
            glBindTexture( GL_TEXTURE_2D, aTextureId );

            setBlendfunction();

            glAlphaFunc( GL_GREATER, 0.2f );
            glEnable( GL_ALPHA_TEST );

            glNormal3f( 0.0f, 0.0f, aIsNormalUp?1.0f:-1.0f );

            glDrawArrays( GL_TRIANGLES, 0, aTriangleContainer->GetVertexSize() );

            glDisable( GL_TEXTURE_2D );
            glDisable( GL_ALPHA_TEST );
            glDisable( GL_BLEND );

            glEndList();

            glDisableClientState( GL_VERTEX_ARRAY );
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );

            delete [] uvArray;
            return listIdx;
        }
    }

    return 0;
}


GLuint CLAYERS_OGL_DISP_LISTS::generate_top_or_bot_triangles(
        const CLAYER_TRIANGLE_CONTAINER *aTriangleContainer,
        bool aIsNormalUp ) const
{
    wxASSERT( aTriangleContainer != NULL );

    wxASSERT( (aTriangleContainer->GetVertexSize() % 3) == 0 );

    // Top and Bot dont have normals array stored in container
    wxASSERT( aTriangleContainer->GetNormalsSize() == 0 );

    if( (aTriangleContainer->GetVertexSize() > 0) &&
        ( (aTriangleContainer->GetVertexSize() % 3) == 0) )
    {
        const GLuint listIdx = glGenLists( 1 );

        if( glIsList( listIdx ) )
        {
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );
            glDisableClientState( GL_COLOR_ARRAY );
            glDisableClientState( GL_NORMAL_ARRAY );
            glEnableClientState( GL_VERTEX_ARRAY );
            glVertexPointer( 3, GL_FLOAT, 0, aTriangleContainer->GetVertexPointer() );

            glNewList( listIdx, GL_COMPILE );

            setBlendfunction();

            glNormal3f( 0.0f, 0.0f, aIsNormalUp?1.0f:-1.0f );

            glDrawArrays( GL_TRIANGLES, 0, aTriangleContainer->GetVertexSize() );

            glDisable( GL_BLEND );
            glEndList();

            glDisableClientState( GL_VERTEX_ARRAY );

            return listIdx;
        }
    }

    return 0;
}


GLuint CLAYERS_OGL_DISP_LISTS::generate_middle_triangles(
        const CLAYER_TRIANGLE_CONTAINER *aTriangleContainer ) const
{
    wxASSERT( aTriangleContainer != NULL );

    // We expect that it is a multiple of 3 vertex
    wxASSERT( (aTriangleContainer->GetVertexSize() % 3) == 0 );

    // We expect that it is a multiple of 6 vertex (because we expect to add quads)
    wxASSERT( (aTriangleContainer->GetVertexSize() % 6) == 0 );

    // We expect that there are normals with same size as vertex
    wxASSERT( aTriangleContainer->GetNormalsSize() == aTriangleContainer->GetVertexSize() );


    if( ( aTriangleContainer->GetVertexSize() > 0 ) &&
        ( (aTriangleContainer->GetVertexSize() % 3) == 0 ) &&
        ( (aTriangleContainer->GetVertexSize() % 6) == 0 ) &&
        ( aTriangleContainer->GetNormalsSize() == aTriangleContainer->GetVertexSize() ) )
    {
        const GLuint listIdx = glGenLists( 1 );

        if( glIsList( listIdx ) )
        {
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );
            glDisableClientState( GL_COLOR_ARRAY );
            glEnableClientState( GL_NORMAL_ARRAY );
            glEnableClientState( GL_VERTEX_ARRAY );
            glVertexPointer( 3, GL_FLOAT, 0, aTriangleContainer->GetVertexPointer() );
            glNormalPointer( GL_FLOAT, 0, aTriangleContainer->GetNormalsPointer() );

            glNewList( listIdx, GL_COMPILE );

            setBlendfunction();

            glDrawArrays( GL_TRIANGLES, 0, aTriangleContainer->GetVertexSize() );

            glDisable( GL_BLEND );
            glEndList();

            glDisableClientState( GL_VERTEX_ARRAY );
            glDisableClientState( GL_NORMAL_ARRAY );

            return listIdx;
        }
    }

    return 0;
}


void CLAYERS_OGL_DISP_LISTS::endTransformation() const
{
    if( m_haveTransformation )
    {
        glPopMatrix();
    }
}


void CLAYERS_OGL_DISP_LISTS::setBlendfunction() const
{
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}


void CLAYERS_OGL_DISP_LISTS::beginTransformation() const
{
    if( m_haveTransformation )
    {
        glPushMatrix();
        glTranslatef( 0.0f, 0.0f, m_zPositionTransformation );
        glScalef( 1.0f, 1.0f, m_zScaleTransformation );
    }
}
