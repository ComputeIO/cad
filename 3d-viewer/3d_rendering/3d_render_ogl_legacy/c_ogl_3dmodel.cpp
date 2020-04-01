/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Oleg Endo <olegendo@gcc.gnu.org>
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
 * @file  c_ogl_3dmodel.cpp
 * @brief
 */

#include <GL/glew.h>
#include "c_ogl_3dmodel.h"
#include "ogl_legacy_utils.h"
#include "../common_ogl/ogl_utils.h"
#include "../3d_math.h"
#include <wx/debug.h>
#include <chrono>

const wxChar * C_OGL_3DMODEL::m_logTrace = wxT( "KI_TRACE_EDA_OGL_3DMODEL" );

void C_OGL_3DMODEL::MakeBbox (const CBBOX& box, unsigned int idx_offset,
                              vertex* vtx_out, GLuint* idx_out,
                              const glm::vec4& color)
{
  vtx_out[0].pos = { box.Min ().x, box.Min ().y, box.Min ().z };
  vtx_out[1].pos = { box.Max ().x, box.Min ().y, box.Min ().z };
  vtx_out[2].pos = { box.Max ().x, box.Max ().y, box.Min ().z };
  vtx_out[3].pos = { box.Min ().x, box.Max ().y, box.Min ().z };

  vtx_out[4].pos = { box.Min ().x, box.Min ().y, box.Max ().z };
  vtx_out[5].pos = { box.Max ().x, box.Min ().y, box.Max ().z };
  vtx_out[6].pos = { box.Max ().x, box.Max ().y, box.Max ().z };
  vtx_out[7].pos = { box.Min ().x, box.Max ().y, box.Max ().z };

  for (unsigned int i = 0; i < 8; ++i)
    vtx_out[i].color = vtx_out[i].cad_color = glm::clamp (color * 255.0f, 0.0f, 255.0f);

  #define bbox_line(vtx_a, vtx_b)\
     do { *idx_out++ = vtx_a + idx_offset; \
          *idx_out++ = vtx_b + idx_offset; } while (0)

  bbox_line (0, 1);
  bbox_line (1, 2);
  bbox_line (2, 3);
  bbox_line (3, 0);

  bbox_line (4, 5);
  bbox_line (5, 6);
  bbox_line (6, 7);
  bbox_line (7, 4);

  bbox_line (0, 4);
  bbox_line (1, 5);
  bbox_line (2, 6);
  bbox_line (3, 7);

  #undef bbox_line
}

C_OGL_3DMODEL::C_OGL_3DMODEL( const S3DMODEL &a3DModel,
                              MATERIAL_MODE aMaterialMode )
{
    wxLogTrace (m_logTrace, wxT ("C_OGL_3DMODEL::C_OGL_3DMODEL %u meshes %u materials"),
                (unsigned int)a3DModel.m_MeshesSize,
                (unsigned int)a3DModel.m_MaterialsSize);

    auto start_time = std::chrono::high_resolution_clock::now ();

    // Validate a3DModel pointers
    wxASSERT( a3DModel.m_Materials != nullptr );
    wxASSERT( a3DModel.m_Meshes != nullptr );
    wxASSERT( a3DModel.m_MaterialsSize > 0 );
    wxASSERT( a3DModel.m_MeshesSize > 0 );

    m_material_mode = aMaterialMode;

    if( (a3DModel.m_Materials == nullptr) || (a3DModel.m_Meshes == nullptr)
        || (a3DModel.m_MaterialsSize == 0) || (a3DModel.m_MeshesSize == 0) )
      return;

    // create empty bbox for each mesh.  it will be updated when the vertices
    // are copied.
    m_meshes_bbox.resize (a3DModel.m_MeshesSize);

    // copy materials for later use during rendering.
    m_materials.reserve (a3DModel.m_MaterialsSize);
    for (unsigned int i = 0; i < a3DModel.m_MaterialsSize; ++i)
        m_materials.emplace_back (a3DModel.m_Materials[i]);

    // build temporary vertex and index buffers for bounding boxes.
    // the first box is the outer box.
    std::vector<vertex> bbox_tmp_vertices ((m_meshes_bbox.size () + 1) * bbox_vtx_count);
    std::vector<GLuint> bbox_tmp_indices ((m_meshes_bbox.size () + 1) * bbox_idx_count);

    // group all meshes by material.
    // for each material create a combined vertex and index buffer.
    // some models might have many sub-meshes.  so iterate over the
    // input meshes only once.
    struct mesh_group
    {
        std::vector<vertex> vertices;
        std::vector<GLuint> indices;
    };

    std::vector<mesh_group> mesh_groups (m_materials.size ());

    for (unsigned int mesh_i = 0; mesh_i < a3DModel.m_MeshesSize; ++mesh_i)
    {
      const auto& mesh = a3DModel.m_Meshes[mesh_i];

      // silently ignore meshes that have invalid material references
      // or invalid geometry.
      if (mesh.m_MaterialIdx >= m_materials.size ()
          || mesh.m_Positions == nullptr
          || mesh.m_FaceIdx == nullptr
          || mesh.m_Normals == nullptr
          || mesh.m_FaceIdxSize == 0
          || mesh.m_VertexSize == 0)
        continue;

      auto& mesh_group = mesh_groups[mesh.m_MaterialIdx];
      auto& material = m_materials[mesh.m_MaterialIdx];

      if (material.is_transparent ())
        m_have_transparent_meshes = true;
      else
        m_have_opaque_meshes = true;

      const unsigned int vtx_offset = mesh_group.vertices.size ();
      mesh_group.vertices.resize (mesh_group.vertices.size () + mesh.m_VertexSize);

      // copy vertex data and update the bounding box.
      // use material color for mesh bounding box or some sort of average
      // vertex color.
      glm::vec3 avg_color = material.m_Diffuse;

      for (unsigned int vtx_i = 0; vtx_i < mesh.m_VertexSize; ++vtx_i)
      {
        m_meshes_bbox[mesh_i].Union( mesh.m_Positions[vtx_i] );

        auto& vtx_out = mesh_group.vertices[vtx_offset + vtx_i];

        vtx_out.pos = mesh.m_Positions[vtx_i];
        vtx_out.nrm = glm::clamp (glm::vec4 (mesh.m_Normals[vtx_i], 1.0f) * 127.0f, -127.0f, 127.0f);

        vtx_out.tex_uv = mesh.m_Texcoords != nullptr
                         ? mesh.m_Texcoords[vtx_i]
                         : glm::vec2 (0);

        if (mesh.m_Color != nullptr)
        {
          avg_color = (avg_color + mesh.m_Color[vtx_i]) * 0.5f;

          vtx_out.color = glm::clamp (glm::vec4 (mesh.m_Color[vtx_i], 1 - material.m_Transparency) * 255.0f, 0.0f, 255.0f);
          vtx_out.cad_color = glm::clamp (glm::vec4 ( MaterialDiffuseToColorCAD( mesh.m_Color[vtx_i] ), 1 ) * 255.0f, 0.0f, 255.0f);
        }
        else
        {
          // the mesh will be rendered with other meshes that might have
          // vertex colors.  thus, we can't enable/disable vertex colors
          // for individual meshes during rendering.

          // if there are no vertex colors, use material color instead.
          vtx_out.color = glm::clamp (glm::vec4 (material.m_Diffuse, 1 - material.m_Transparency) * 255.0f, 0.0f, 255.0f);
          vtx_out.cad_color = glm::clamp (glm::vec4 ( MaterialDiffuseToColorCAD( material.m_Diffuse ), 1 ) * 255.0f, 0.0f, 255.0f);
        }
      }

      if (m_meshes_bbox[mesh_i].IsInitialized ())
      {
        // generate geometry for the bounding box
        MakeBbox (m_meshes_bbox[mesh_i], (mesh_i + 1) * bbox_vtx_count,
                  &bbox_tmp_vertices[(mesh_i + 1) * bbox_vtx_count],
                  &bbox_tmp_indices[(mesh_i + 1) * bbox_idx_count],
                  { avg_color, 1.0f });

        // bump the outer bounding box
        m_model_bbox.Union( m_meshes_bbox[mesh_i] );
      }


      // append indices of this mesh to the mesh group.
      const unsigned int idx_offset = mesh_group.indices.size ();
      unsigned int use_idx_count = mesh.m_FaceIdxSize;
      if (use_idx_count % 3 != 0)
      {
        wxLogTrace (m_logTrace, wxT ("  index count %u not multiple of 3, truncating"),
                    (unsigned int)use_idx_count);
        use_idx_count = (use_idx_count / 3) * 3;
      }
      mesh_group.indices.resize (mesh_group.indices.size () + use_idx_count);

      for (unsigned int idx_i = 0; idx_i < use_idx_count; ++idx_i)
      {
        if (mesh.m_FaceIdx[idx_i] >= mesh.m_VertexSize)
        {
          wxLogTrace (m_logTrace, wxT (" index %u out of range (%u)"),
                      (unsigned int)mesh.m_FaceIdx[idx_i], (unsigned int)mesh.m_VertexSize);

          // FIXME: should skip this triangle
        }

        mesh_group.indices[idx_offset + idx_i] = mesh.m_FaceIdx[idx_i] + vtx_offset;
      }
    }

    // generate geometry for the outer bounding box
    if (m_model_bbox.IsInitialized ())
      MakeBbox (m_model_bbox, 0, &bbox_tmp_vertices[0], &bbox_tmp_indices[0],
                { 0.0f, 1.0f, 0.0f, 1.0f });

    // create bounding box buffers
    glGenBuffers (1, &m_bbox_vertex_buffer);
    glBindBuffer (GL_ARRAY_BUFFER, m_bbox_vertex_buffer);
    glBufferData (GL_ARRAY_BUFFER, sizeof (vertex) * bbox_tmp_vertices.size (),
                  bbox_tmp_vertices.data (), GL_STATIC_DRAW);

    glGenBuffers (1, &m_bbox_index_buffer);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_bbox_index_buffer);

    if (bbox_tmp_vertices.size () <= std::numeric_limits<GLushort>::max ())
    {
      m_bbox_index_buffer_type = GL_UNSIGNED_SHORT;

      auto u16buf = std::make_unique<GLushort[]> (bbox_tmp_indices.size ());
      for (unsigned int i = 0; i < bbox_tmp_indices.size (); ++i)
        u16buf[i] = (GLushort)bbox_tmp_indices[i];

      glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (GLushort) * bbox_tmp_indices.size (),
                    u16buf.get (), GL_STATIC_DRAW);
    }
    else
    {
      m_bbox_index_buffer_type = GL_UNSIGNED_INT;
      glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (GLuint) * bbox_tmp_indices.size (),
                  bbox_tmp_indices.data (), GL_STATIC_DRAW);
    }


    // merge the mesh group geometry data.
    unsigned int total_vertex_count = 0;
    unsigned int total_index_count = 0;

    for (auto& mg : mesh_groups)
    {
      total_vertex_count += mg.vertices.size ();
      total_index_count += mg.indices.size ();
    }

    wxLogTrace (m_logTrace, wxT ("  total %u vertices, %u indices"),
                total_vertex_count, total_index_count);

    glGenBuffers (1, &m_vertex_buffer);
    glBindBuffer (GL_ARRAY_BUFFER, m_vertex_buffer);
    glBufferData (GL_ARRAY_BUFFER, sizeof (vertex) * total_vertex_count,
                  nullptr, GL_STATIC_DRAW);

    unsigned int idx_size = 0;

    if (total_vertex_count <= std::numeric_limits<GLushort>::max ())
    {
      m_index_buffer_type = GL_UNSIGNED_SHORT;
      idx_size = sizeof (GLushort);
    }
    else
    {
      m_index_buffer_type = GL_UNSIGNED_INT;
      idx_size = sizeof (GLuint);
    }

    // temporary index buffer which will contain either GLushort or GLuint
    // type indices.  allocate with a bit of meadow at the end.
    auto tmp_idx = std::make_unique<GLuint[]> ((idx_size * total_index_count + 8) / sizeof (GLuint));

    unsigned int prev_vtx_count = 0;
    unsigned int idx_offset = 0;
    unsigned int vtx_offset = 0;

    for (unsigned int mg_i = 0; mg_i < mesh_groups.size (); ++mg_i)
    {
      auto& mg = mesh_groups[mg_i];
      auto& mat = m_materials[mg_i];

      if (m_index_buffer_type == GL_UNSIGNED_SHORT)
      {
        auto* idx_out = (GLushort*)((uintptr_t)tmp_idx.get () + idx_offset);
        for (auto idx : mg.indices)
          *idx_out++ = (GLushort)(idx + prev_vtx_count);
      }
      else if (m_index_buffer_type == GL_UNSIGNED_INT)
      {
        auto* idx_out = (GLuint*)((uintptr_t)tmp_idx.get () + idx_offset);
        for (auto idx : mg.indices)
          *idx_out++ = (GLuint)(idx + prev_vtx_count);
      }

      glBufferSubData (GL_ARRAY_BUFFER,
                       vtx_offset,
                       mg.vertices.size () * sizeof (vertex),
                       mg.vertices.data ());

      mat.render_idx_buffer_offset = idx_offset;
      mat.render_idx_count = mg.indices.size ();

      prev_vtx_count += mg.vertices.size ();
      idx_offset += mg.indices.size () * idx_size;
      vtx_offset += mg.vertices.size () * sizeof (vertex);
    }

    glGenBuffers (1, &m_index_buffer);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, idx_size * total_index_count,
                  tmp_idx.get (), GL_STATIC_DRAW);

    glBindBuffer (GL_ARRAY_BUFFER, 0);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);

    auto end_time = std::chrono::high_resolution_clock::now ();

    wxLogTrace (m_logTrace, wxT ("  loaded in %u ms\n"),
                (unsigned int)std::chrono::duration_cast<std::chrono::milliseconds> (end_time - start_time).count ());
}

void C_OGL_3DMODEL::BeginDrawMulti (void)
{
  glEnableClientState (GL_VERTEX_ARRAY);
  glEnableClientState (GL_NORMAL_ARRAY);
  glEnableClientState (GL_COLOR_ARRAY);
  glEnableClientState (GL_TEXTURE_COORD_ARRAY);

  glEnable (GL_COLOR_MATERIAL);
  glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

void C_OGL_3DMODEL::EndDrawMulti (void)
{
  glDisable( GL_COLOR_MATERIAL );
  glDisableClientState (GL_VERTEX_ARRAY);
  glDisableClientState (GL_NORMAL_ARRAY);
  glDisableClientState (GL_COLOR_ARRAY);
  glDisableClientState (GL_TEXTURE_COORD_ARRAY);

  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
}


void C_OGL_3DMODEL::Draw (bool transparent) const
{
  glBindBuffer (GL_ARRAY_BUFFER, m_vertex_buffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);

  glVertexPointer (3, GL_FLOAT, sizeof (vertex), (const void*)offsetof (vertex, pos));
  glNormalPointer (GL_BYTE, sizeof (vertex), (const void*)offsetof (vertex, nrm));
  glColorPointer (4, GL_UNSIGNED_BYTE, sizeof (vertex), (const void*)
                  (m_material_mode == MATERIAL_MODE::CAD_MODE
                   ? offsetof (vertex, cad_color)
                   : offsetof (vertex, color)));
  glTexCoordPointer (2, GL_FLOAT, sizeof (vertex), (const void*)offsetof (vertex, tex_uv));

  // BeginDrawMulti ();

  for (auto& mat : m_materials)
  {
    if (mat.is_transparent () != transparent)
      continue;

    switch (m_material_mode)
    {
    case MATERIAL_MODE::NORMAL:
      OGL_SetMaterial (mat);
      break;

    case MATERIAL_MODE::DIFFUSE_ONLY:
      OGL_SetDiffuseOnlyMaterial( mat.m_Diffuse );
      break;

    case MATERIAL_MODE::CAD_MODE:
      OGL_SetDiffuseOnlyMaterial(MaterialDiffuseToColorCAD(mat.m_Diffuse));
      break;

    default:
      break;
    }

    glDrawElements (GL_TRIANGLES, mat.render_idx_count, m_index_buffer_type,
                    (const void*)(uintptr_t)mat.render_idx_buffer_offset);
  }

  // EndDrawMulti ();
}

C_OGL_3DMODEL::~C_OGL_3DMODEL()
{
  glDeleteBuffers (1, &m_vertex_buffer);
  glDeleteBuffers (1, &m_index_buffer);
  glDeleteBuffers (1, &m_bbox_vertex_buffer);
  glDeleteBuffers (1, &m_bbox_index_buffer);
}


void C_OGL_3DMODEL::Draw_bbox() const
{
  glBindBuffer (GL_ARRAY_BUFFER, m_bbox_vertex_buffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_bbox_index_buffer);

  glVertexPointer (3, GL_FLOAT, sizeof (vertex), (const void*)offsetof (vertex, pos));
  glColorPointer (4, GL_UNSIGNED_BYTE, sizeof (vertex), (const void*)offsetof (vertex, color));

  glDrawElements (GL_LINES, bbox_idx_count, m_bbox_index_buffer_type,
                  (const void*)(uintptr_t)0);
}


void C_OGL_3DMODEL::Draw_bboxes() const
{
  glBindBuffer (GL_ARRAY_BUFFER, m_bbox_vertex_buffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_bbox_index_buffer);

  glVertexPointer (3, GL_FLOAT, sizeof (vertex), (const void*)offsetof (vertex, pos));
  glColorPointer (4, GL_UNSIGNED_BYTE, sizeof (vertex), (const void*)offsetof (vertex, color));

  unsigned int idx_size = m_bbox_index_buffer_type == GL_UNSIGNED_SHORT
                          ? sizeof (GLushort) : sizeof (GLuint);

  glDrawElements (GL_LINES, bbox_idx_count * m_meshes_bbox.size (), m_bbox_index_buffer_type,
                  (const void*)(uintptr_t)(bbox_idx_count * idx_size));
}

