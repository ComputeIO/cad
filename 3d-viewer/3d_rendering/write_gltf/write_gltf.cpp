#include <wx/mstream.h>

#include "streamwrapper.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

#include "../../3d_rendering/3d_render_ogl_legacy/c3d_render_ogl_legacy.h"
#include "../../3d_rendering/cimage.h"
#include "../../3d_rendering/buffers_debug.h"

#include "write_gltf.h"

/**
 * @brief CircleImage - Returns the bytes of a circle in PNG format for use as
 * a segment end alpha texture mask.
 */
std::vector<unsigned char> CircleImage() {
    CIMAGE *circleImage = new CIMAGE( SIZE_OF_CIRCLE_TEXTURE, SIZE_OF_CIRCLE_TEXTURE );

    circleImage->CircleFilled( (SIZE_OF_CIRCLE_TEXTURE / 2) - 0,
                               (SIZE_OF_CIRCLE_TEXTURE / 2) - 0,
                               (SIZE_OF_CIRCLE_TEXTURE / 2) - 10,
                               0xFF );

    wxMemoryOutputStream stream;
    circleImage->SaveAsPNGStream(stream);
    int circleImage_Size = stream.GetSize();
    auto circleImage_data = std::vector<unsigned char>();
    circleImage_data.resize(circleImage_Size);

    stream.CopyTo(circleImage_data.data(), circleImage_Size);

    delete circleImage;
    circleImage = 0;

    return circleImage_data;
}

void WriteGLTF( const CLAYER_TRIANGLES* geometry, const std::string& gltf_name ) {
    int top_length = geometry->m_layer_top_triangles->GetVertexSize();
    int top_bytes = top_length * sizeof(SFVEC3F);

    int mid_length = geometry->m_layer_middle_contourns_quads->GetVertexSize();
    int mid_bytes = mid_length * sizeof(SFVEC3F);

    int top_seg_length = geometry->m_layer_top_segment_ends->GetVertexSize();
    int top_seg_bytes = top_seg_length * sizeof(SFVEC3F);

    SFVEC2F *top_seg_uv_data = new SFVEC2F[top_seg_length];

    for( int i = 0; i < top_seg_length; i += 3 ) {
        top_seg_uv_data[i + 0] = SFVEC2F( 1.0f, 0.0f );
        top_seg_uv_data[i + 1] = SFVEC2F( 0.0f, 1.0f );
        top_seg_uv_data[i + 2] = SFVEC2F( 0.0f, 0.0f );
    }

    int top_seg_uv_length = top_seg_length;
    int top_seg_uv_bytes = top_seg_uv_length * sizeof(SFVEC2F);

    std::vector<unsigned char> circle_tex_data = CircleImage();

    tinygltf::Model model = tinygltf::Model();
    tinygltf::Scene scene = tinygltf::Scene();
    scene.nodes.push_back(0);
    scene.nodes.push_back(1);
    scene.nodes.push_back(2);
    model.scenes.push_back(scene);
    tinygltf::Node top_face = tinygltf::Node();
    top_face.mesh = 0;
    model.nodes.push_back(top_face);
    tinygltf::Node side_face = tinygltf::Node();
    side_face.mesh = 1;
    model.nodes.push_back(side_face);
    tinygltf::Node top_circles = tinygltf::Node();
    top_circles.mesh = 2;
    model.nodes.push_back(top_circles);
    tinygltf::Mesh top_face_mesh = tinygltf::Mesh();
    tinygltf::Primitive top_face_primitive = tinygltf::Primitive();
    top_face_primitive.attributes.insert(std::pair<std::string, int>("POSITION", 0));
    top_face_primitive.mode = TINYGLTF_MODE_TRIANGLES;
    top_face_mesh.primitives.push_back(top_face_primitive);
    model.meshes.push_back(top_face_mesh);
    tinygltf::Mesh side_face_mesh = tinygltf::Mesh();
    tinygltf::Primitive side_face_primitive = tinygltf::Primitive();
    side_face_primitive.attributes.insert(std::pair<std::string, int>("POSITION", 1));
    side_face_primitive.attributes.insert(std::pair<std::string, int>("NORMAL", 2));
    side_face_primitive.mode = TINYGLTF_MODE_TRIANGLES;
    side_face_mesh.primitives.push_back(side_face_primitive);
    model.meshes.push_back(side_face_mesh);
    tinygltf::Mesh top_circles_mesh = tinygltf::Mesh();
    tinygltf::Primitive top_circles_primitive = tinygltf::Primitive();
    top_circles_primitive.attributes.insert(std::pair<std::string, int>("POSITION", 3));
    top_circles_primitive.attributes.insert(std::pair<std::string, int>("TEXCOORD_0", 4));
    top_circles_primitive.material = 0;
    top_circles_primitive.mode = TINYGLTF_MODE_TRIANGLES;
    top_circles_mesh.primitives.push_back(top_circles_primitive);
    model.meshes.push_back(top_circles_mesh);
    tinygltf::Buffer buffer = tinygltf::Buffer();
    const unsigned char* top_vertices_data = (const unsigned char*) geometry->m_layer_top_triangles->GetVertexPointer();
    std::copy(&top_vertices_data[0], &top_vertices_data[top_bytes], std::back_inserter(buffer.data));
    const unsigned char* mid_vertices_data = (const unsigned char*) geometry->m_layer_middle_contourns_quads->GetVertexPointer();
    std::copy(&mid_vertices_data[0], &mid_vertices_data[mid_bytes], std::back_inserter(buffer.data));
    const unsigned char* mid_normals_data = (const unsigned char*) geometry->m_layer_middle_contourns_quads->GetNormalsPointer();
    std::copy(&mid_normals_data[0], &mid_normals_data[mid_bytes], std::back_inserter(buffer.data));
    const unsigned char* top_circles_data = (const unsigned char*) geometry->m_layer_top_segment_ends->GetVertexPointer();
    std::copy(&top_circles_data[0], &top_circles_data[top_seg_bytes], std::back_inserter(buffer.data));
    const unsigned char* top_circles_uv_data = (const unsigned char*) top_seg_uv_data;
    std::copy(&top_circles_uv_data[0], &top_circles_uv_data[top_seg_uv_bytes], std::back_inserter(buffer.data));
    std::copy(circle_tex_data.begin(), circle_tex_data.end(), std::back_inserter(buffer.data));
    model.buffers.push_back(buffer);
    tinygltf::BufferView top_vertices = tinygltf::BufferView();
    top_vertices.buffer = 0;
    top_vertices.byteOffset = 0;
    top_vertices.byteLength = top_bytes;
    top_vertices.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(top_vertices);
    tinygltf::BufferView mid_vertices = tinygltf::BufferView();
    mid_vertices.buffer = 0;
    mid_vertices.byteOffset = top_bytes;
    mid_vertices.byteLength = mid_bytes;
    mid_vertices.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(mid_vertices);
    tinygltf::BufferView mid_normals = tinygltf::BufferView();
    mid_normals.buffer = 0;
    mid_normals.byteOffset = top_bytes + mid_bytes;
    mid_normals.byteLength = mid_bytes;
    mid_normals.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(mid_normals);
    tinygltf::BufferView top_seg_vertices = tinygltf::BufferView();
    top_seg_vertices.buffer = 0;
    top_seg_vertices.byteOffset = top_bytes + mid_bytes + mid_bytes;
    top_seg_vertices.byteLength = top_seg_bytes;
    top_seg_vertices.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(top_seg_vertices);
    tinygltf::BufferView top_seg_uv = tinygltf::BufferView();
    top_seg_uv.buffer = 0;
    top_seg_uv.byteOffset = top_bytes + mid_bytes + mid_bytes + top_seg_bytes;
    top_seg_uv.byteLength = top_seg_uv_bytes;
    top_seg_uv.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(top_seg_uv);
    tinygltf::BufferView circle_tex_bufview = tinygltf::BufferView();
    circle_tex_bufview.buffer = 0;
    circle_tex_bufview.byteOffset = top_bytes + mid_bytes + mid_bytes + top_seg_bytes + top_seg_uv_bytes;
    circle_tex_bufview.byteLength = circle_tex_data.size();
    circle_tex_bufview.target = 0;
    model.bufferViews.push_back(circle_tex_bufview);
    tinygltf::Accessor top_vertices_accessor = tinygltf::Accessor();
    top_vertices_accessor.bufferView = 0;
    top_vertices_accessor.byteOffset = 0;
    top_vertices_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    top_vertices_accessor.count = top_length;
    top_vertices_accessor.type = TINYGLTF_TYPE_VEC3;
    model.accessors.push_back(top_vertices_accessor);
    tinygltf::Accessor mid_vertices_accessor = tinygltf::Accessor();
    mid_vertices_accessor.bufferView = 1;
    mid_vertices_accessor.byteOffset = 0;
    mid_vertices_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    mid_vertices_accessor.count = mid_length;
    mid_vertices_accessor.type = TINYGLTF_TYPE_VEC3;
    model.accessors.push_back(mid_vertices_accessor);
    tinygltf::Accessor mid_normals_accessor = tinygltf::Accessor();
    mid_normals_accessor.bufferView = 2;
    mid_normals_accessor.byteOffset = 0;
    mid_normals_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    mid_normals_accessor.count = mid_length;
    mid_normals_accessor.type = TINYGLTF_TYPE_VEC3;
    model.accessors.push_back(mid_normals_accessor);
    tinygltf::Accessor top_seg_vertices_accessor = tinygltf::Accessor();
    top_seg_vertices_accessor.bufferView = 3;
    top_seg_vertices_accessor.byteOffset = 0;
    top_seg_vertices_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    top_seg_vertices_accessor.count = top_seg_length;
    top_seg_vertices_accessor.type = TINYGLTF_TYPE_VEC3;
    model.accessors.push_back(top_seg_vertices_accessor);
    tinygltf::Accessor top_seg_uv_accessor = tinygltf::Accessor();
    top_seg_uv_accessor.bufferView = 4;
    top_seg_uv_accessor.byteOffset = 0;
    top_seg_uv_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    top_seg_uv_accessor.count = top_seg_uv_length;
    top_seg_uv_accessor.type = TINYGLTF_TYPE_VEC2;
    model.accessors.push_back(top_seg_uv_accessor);
    tinygltf::Material circle_tex_material = tinygltf::Material();
    tinygltf::PbrMetallicRoughness circle_tex_pbrmr = tinygltf::PbrMetallicRoughness();
    circle_tex_pbrmr.baseColorFactor = std::vector<double>{ 1.0, 1.0, 1.0, 1.0 };
    tinygltf::TextureInfo circle_tex_base_color_texture = tinygltf::TextureInfo();
    circle_tex_base_color_texture.index = 0;
    circle_tex_base_color_texture.texCoord = 0;
    circle_tex_material.alphaMode = "MASK";
    circle_tex_material.alphaCutoff = 0.01;
    circle_tex_pbrmr.baseColorTexture = circle_tex_base_color_texture;
    circle_tex_material.pbrMetallicRoughness = circle_tex_pbrmr;
    model.materials.push_back(circle_tex_material);
    tinygltf::Texture circle_tex = tinygltf::Texture();
    circle_tex.sampler = 0;
    circle_tex.source = 0;
    model.textures.push_back(circle_tex);
    tinygltf::Image circle_image = tinygltf::Image();
    circle_image.bufferView = 5;
    circle_image.mimeType = "image/png";
    model.images.push_back(circle_image);
    tinygltf::Sampler circle_tex_sampler = tinygltf::Sampler();
    circle_tex_sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    circle_tex_sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
    circle_tex_sampler.wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
    circle_tex_sampler.wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
    model.samplers.push_back(circle_tex_sampler);
    model.defaultScene = 0;
    model.asset.version = "2.0";

    tinygltf::TinyGLTF loader;
    loader.WriteGltfSceneToFile(&model, gltf_name + ".glb", false, true, false, true);
}
