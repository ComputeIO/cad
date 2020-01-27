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

#ifndef CLAYERS_OGL_DISP_LISTS_H_
#define CLAYERS_OGL_DISP_LISTS_H_

#include <geom3d/clayer_triangles.h>
#include "../../common_ogl/openGL_includes.h"
#include <plugins/3dapi/xv3d_types.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <vector>
#include <mutex>

/**
 * @brief The CLAYERS_OGL_DISP_LISTS class stores the openGL display lists to
 * related with a layer
 */
class CLAYERS_OGL_DISP_LISTS
{
public:
    /**
     * @brief CLAYERS_OGL_DISP_LISTS - Creates the display lists for a layer
     * @param aLayerTriangles: contains the layers array of vertex to render to
     *                         display lists
     * @param aTextureIndexForSegEnds: texture index to be used by segment ends.
     *                                 It is a black and white squared texture
     *                                 with a center circle diameter of the size
     *                                 of the texture.
     */
    CLAYERS_OGL_DISP_LISTS( const CLAYER_TRIANGLES &aLayerTriangles,
                            GLuint aTextureIndexForSegEnds,
                            float aZBot,
                            float aZTop );

    /**
     * @brief ~CLAYERS_OGL_DISP_LISTS - Destroy this class while free the display
     * lists from GPU mem
     */
    ~CLAYERS_OGL_DISP_LISTS();

    /**
     * @brief DrawTopAndMiddle - This function calls the display lists for the
     * top elements and middle contourns
     */
    void DrawTopAndMiddle() const;

    /**
     * @brief DrawBotAndMiddle - This function calls the display lists for the
     * botton elements and middle contourns
     */
    void DrawBotAndMiddle() const;

    /**
     * @brief DrawTop - This function calls the display lists for the top elements
     */
    void DrawTop() const;

    /**
     * @brief DrawBot - This function calls the display lists for the botton elements
     */
    void DrawBot() const;

    /**
     * @brief DrawMiddle - This function calls the display lists for the middle
     * elements
     */
    void DrawMiddle() const;

    /**
     * @brief DrawAll - This function calls all the display lists
     */
    void DrawAll( bool aDrawMiddle = true ) const;

    /**
     * @brief DrawAllCameraCulled - Draw all layers if they are visible by the camera.
     * i.e.: if camera position is above the layer. This only works because the
     * board is centered and the planes are always perpendicular to the Z axis.
     * @param zCameraPos: camera z position
     */
    void DrawAllCameraCulled( float zCameraPos, bool aDrawMiddle = true ) const;

    void DrawAllCameraCulledSubtractLayer( const CLAYERS_OGL_DISP_LISTS *aLayerToSubtractA,
                                           const CLAYERS_OGL_DISP_LISTS *aLayerToSubtractB,
                                           bool aDrawMiddle = true ) const;

    void ApplyScalePosition( float aZposition, float aZscale );

    void ClearScalePosition() { m_haveTransformation = false; }

    void SetItIsTransparent( bool aSetTransparent );

    float GetZBot() const { return m_zBot; }
    float GetZTop() const { return m_zTop; }

private:
    GLuint generate_top_or_bot_seg_ends( const CLAYER_TRIANGLE_CONTAINER * aTriangleContainer,
                                         bool aIsNormalUp,
                                         GLuint aTextureId ) const;

    GLuint generate_top_or_bot_triangles( const CLAYER_TRIANGLE_CONTAINER * aTriangleContainer,
                                          bool aIsNormalUp ) const;

    GLuint generate_middle_triangles( const CLAYER_TRIANGLE_CONTAINER * aTriangleContainer ) const;

    void beginTransformation() const;
    void endTransformation() const;

    void setBlendfunction() const;

private:
    float   m_zBot;
    float   m_zTop;
    GLuint  m_layer_top_segment_ends;
    GLuint  m_layer_top_triangles;
    GLuint  m_layer_middle_contourns_quads;
    GLuint  m_layer_bot_triangles;
    GLuint  m_layer_bot_segment_ends;

    bool    m_haveTransformation;
    float   m_zPositionTransformation;
    float   m_zScaleTransformation;

    bool    m_draw_it_transparent;
};

#endif // CLAYERS_OGL_DISP_LISTS_H_
