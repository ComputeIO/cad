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

/**
 * @file  clayer_triangles.cpp
 * @brief
 */


#include "geom3d/clayer_triangles.h"
#include <wx/debug.h>   // For the wxASSERT
#include <mutex>
#include <thread>
#include <atomic>


CLAYER_TRIANGLE_CONTAINER::CLAYER_TRIANGLE_CONTAINER( unsigned int aNrReservedTriangles,
                                                      bool aReserveNormals )
{
    wxASSERT( aNrReservedTriangles > 0 );

    m_vertexs.clear();
    m_normals.clear();

    m_vertexs.reserve( aNrReservedTriangles * 3 );

    if( aReserveNormals )
        m_normals.reserve( aNrReservedTriangles * 3 );
}


void CLAYER_TRIANGLE_CONTAINER::Reserve_More( unsigned int aNrReservedTriangles,
                                              bool aReserveNormals )
{
    m_vertexs.reserve( m_vertexs.size() + aNrReservedTriangles * 3 );

    if( aReserveNormals )
        m_normals.reserve( m_normals.size() + aNrReservedTriangles * 3 );
}


void CLAYER_TRIANGLE_CONTAINER::AddQuad( const SFVEC3F &aV1,
                                         const SFVEC3F &aV2,
                                         const SFVEC3F &aV3,
                                         const SFVEC3F &aV4 )
{
    m_vertexs.push_back( aV1 );
    m_vertexs.push_back( aV2 );
    m_vertexs.push_back( aV3 );

    m_vertexs.push_back( aV3 );
    m_vertexs.push_back( aV4 );
    m_vertexs.push_back( aV1 );
}


void CLAYER_TRIANGLE_CONTAINER::AddTriangle( const SFVEC3F &aV1,
                                             const SFVEC3F &aV2,
                                             const SFVEC3F &aV3 )
{
    m_vertexs.push_back( aV1 );
    m_vertexs.push_back( aV2 );
    m_vertexs.push_back( aV3 );
}


void CLAYER_TRIANGLE_CONTAINER::AddNormal( const SFVEC3F &aN1,
                                           const SFVEC3F &aN2,
                                           const SFVEC3F &aN3 )
{
    m_normals.push_back( aN1 );
    m_normals.push_back( aN2 );
    m_normals.push_back( aN3 );
}

void CLAYER_TRIANGLE_CONTAINER::AddNormal( const SFVEC3F &aN1,
                                           const SFVEC3F &aN2,
                                           const SFVEC3F &aN3,
                                           const SFVEC3F &aN4 )
{
    m_normals.push_back( aN1 );
    m_normals.push_back( aN2 );
    m_normals.push_back( aN3 );

    m_normals.push_back( aN3 );
    m_normals.push_back( aN4 );
    m_normals.push_back( aN1 );
}


CLAYER_TRIANGLES::CLAYER_TRIANGLES( unsigned int aNrReservedTriangles )
{
    wxASSERT( aNrReservedTriangles > 0 );

    m_layer_top_segment_ends        = new CLAYER_TRIANGLE_CONTAINER( aNrReservedTriangles,
                                                                     false );
    m_layer_top_triangles           = new CLAYER_TRIANGLE_CONTAINER( aNrReservedTriangles,
                                                                     false );
    m_layer_middle_contourns_quads  = new CLAYER_TRIANGLE_CONTAINER( aNrReservedTriangles,
                                                                     true  );
    m_layer_bot_triangles           = new CLAYER_TRIANGLE_CONTAINER( aNrReservedTriangles,
                                                                     false );
    m_layer_bot_segment_ends        = new CLAYER_TRIANGLE_CONTAINER( aNrReservedTriangles,
                                                                     false );
}


CLAYER_TRIANGLES::~CLAYER_TRIANGLES()
{
    delete m_layer_top_segment_ends;
    m_layer_top_segment_ends = 0;

    delete m_layer_top_triangles;
    m_layer_top_triangles = 0;

    delete m_layer_middle_contourns_quads;
    m_layer_middle_contourns_quads = 0;

    delete m_layer_bot_triangles;
    m_layer_bot_triangles = 0;

    delete m_layer_bot_segment_ends;
    m_layer_bot_segment_ends = 0;
}


void CLAYER_TRIANGLES::AddToMiddleContourns( const std::vector< SFVEC2F > &aContournPoints,
                                             float zBot,
                                             float zTop,
                                             bool aInvertFaceDirection )
{
    if( aContournPoints.size() >= 4 )
    {
        // Calculate normals of each segment of the contourn
        std::vector< SFVEC2F > contournNormals;

        contournNormals.clear();
        contournNormals.resize( aContournPoints.size() - 1 );

        if( aInvertFaceDirection )
        {
            for( unsigned int i = 0; i < ( aContournPoints.size() - 1 ); ++i )
            {
                const SFVEC2F &v0 = aContournPoints[i + 0];
                const SFVEC2F &v1 = aContournPoints[i + 1];
                const SFVEC2F n = glm::normalize( v1 - v0 );

                contournNormals[i] = SFVEC2F( n.y,-n.x );
            }
        }
        else
        {
            for( unsigned int i = 0; i < ( aContournPoints.size() - 1 ); ++i )
            {
                const SFVEC2F &v0 = aContournPoints[i + 0];
                const SFVEC2F &v1 = aContournPoints[i + 1];
                const SFVEC2F n = glm::normalize( v1 - v0 );

                contournNormals[i] = SFVEC2F( -n.y, n.x );
            }
        }


        if( aInvertFaceDirection )
            std::swap( zBot, zTop );

        const unsigned int nContournsToProcess = ( aContournPoints.size() - 1 );

        for( unsigned int i = 0; i < nContournsToProcess; ++i )
        {
            SFVEC2F lastNormal;

            if( i > 0 )
                lastNormal = contournNormals[i - 1];
            else
                lastNormal = contournNormals[nContournsToProcess - 1];

            SFVEC2F n0 = contournNormals[i];

            // Only interpolate the normal if the angle is closer
            if( glm::dot( n0, lastNormal ) > 0.5f )
                n0 = glm::normalize( n0 + lastNormal );

            SFVEC2F nextNormal;

            if( i < (nContournsToProcess - 1) )
                nextNormal = contournNormals[i + 1];
            else
                nextNormal = contournNormals[0];

            SFVEC2F n1 = contournNormals[i];

            if( glm::dot( n1, nextNormal ) > 0.5f )
                n1 = glm::normalize( n1 + nextNormal );

            const SFVEC3F n3d0 = SFVEC3F( n0.x, n0.y, 0.0f );
            const SFVEC3F n3d1 = SFVEC3F( n1.x, n1.y, 0.0f );

            const SFVEC2F &v0 = aContournPoints[i + 0];
            const SFVEC2F &v1 = aContournPoints[i + 1];

            {
                std::lock_guard<std::mutex> lock( m_middle_layer_lock );
                m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v0.x, v0.y, zTop ),
                                                         SFVEC3F( v1.x, v1.y, zTop ),
                                                         SFVEC3F( v1.x, v1.y, zBot ),
                                                         SFVEC3F( v0.x, v0.y, zBot ) );

                m_layer_middle_contourns_quads->AddNormal( n3d0, n3d1, n3d1, n3d0 );
            }
        }
    }
}


void CLAYER_TRIANGLES::AddToMiddleContourns( const SHAPE_LINE_CHAIN &outlinePath,
                                             float zBot,
                                             float zTop,
                                             double aBiuTo3Du,
                                             bool aInvertFaceDirection )
{
    std::vector< SFVEC2F >contournPoints;

    contournPoints.clear();
    contournPoints.reserve( outlinePath.PointCount() + 2 );

    const VECTOR2I &firstV = outlinePath.CPoint( 0 );

    SFVEC2F lastV = SFVEC2F( firstV.x * aBiuTo3Du,
                            -firstV.y * aBiuTo3Du );

    contournPoints.push_back( lastV );

    for( unsigned int i = 1; i < (unsigned int)outlinePath.PointCount(); ++i )
    {
        const VECTOR2I & v = outlinePath.CPoint( i );

        const SFVEC2F vf = SFVEC2F(  v.x * aBiuTo3Du,
                                    -v.y * aBiuTo3Du );

        if( vf != lastV ) // Do not add repeated points
        {
            lastV = vf;
            contournPoints.push_back( vf );
        }
    }

    // Add first position fo the list to close the path
    if( lastV != contournPoints[0] )
        contournPoints.push_back( contournPoints[0] );

    AddToMiddleContourns( contournPoints, zBot, zTop, aInvertFaceDirection );
}


void CLAYER_TRIANGLES::AddToMiddleContourns( const SHAPE_POLY_SET &aPolySet,
                                             float zBot,
                                             float zTop,
                                             double aBiuTo3Du,
                                             bool aInvertFaceDirection )
{
    if( aPolySet.OutlineCount() == 0 )
        return;

    // Calculate an estimation of points to reserve
    unsigned int nrContournPointsToReserve = 0;

    for( int i = 0; i < aPolySet.OutlineCount(); ++i )
    {
        const SHAPE_LINE_CHAIN& pathOutline = aPolySet.COutline( i );

        nrContournPointsToReserve += pathOutline.PointCount();

        for( int h = 0; h < aPolySet.HoleCount( i ); ++h )
        {
            const SHAPE_LINE_CHAIN &hole = aPolySet.CHole( i, h );

            nrContournPointsToReserve += hole.PointCount();
        }
    }

    // Request to reserve more space
    m_layer_middle_contourns_quads->Reserve_More( nrContournPointsToReserve * 2,
                                                  true );

    for( int i = 0; i < aPolySet.OutlineCount(); i++ )
    {
        // Add outline
        const SHAPE_LINE_CHAIN& pathOutline = aPolySet.COutline( i );

        AddToMiddleContourns( pathOutline, zBot, zTop, aBiuTo3Du, aInvertFaceDirection );

        // Add holes for this outline
        for( int h = 0; h < aPolySet.HoleCount( i ); ++h )
        {
            const SHAPE_LINE_CHAIN &hole = aPolySet.CHole( i, h );
            AddToMiddleContourns( hole, zBot, zTop, aBiuTo3Du, aInvertFaceDirection );
        }
    }
}
