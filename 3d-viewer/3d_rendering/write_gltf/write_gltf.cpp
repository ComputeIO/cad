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

std::vector<double> vec3_elementwise_min(const float* elements, size_t length) {
    std::vector<float> min = std::vector<float>{elements[0], elements[1], elements[2]};
    for (unsigned int i = 3; i < length * 3; i += 3) {
        const float* element = elements + i;
        if (element[0] < min[0]) {
            min[0] = element[0];
        }
        if (element[1] < min[1]) {
            min[1] = element[1];
        }
        if (element[2] < min[2]) {
            min[2] = element[2];
        }
    }
    return std::vector<double>{min[0], min[1], min[2]};
}

std::vector<double> vec3_elementwise_max(const float* elements, size_t length) {
    std::vector<float> max = std::vector<float>{elements[0], elements[1], elements[2]};
    for (unsigned int i = 3; i < length * 3; i += 3) {
        const float* element = elements + i;
        if (element[0] > max[0]) {
            max[0] = element[0];
        }
        if (element[1] > max[1]) {
            max[1] = element[1];
        }
        if (element[2] > max[2]) {
            max[2] = element[2];
        }
    }
    return std::vector<double>{max[0], max[1], max[2]};
}

/**
 * @brief PushUntexturedNode - Adds a new mesh node to the GLTF model from the
 * given triangle container. If add_normals is false, the mesh is assumed to be
 * flat.
 */
void PushUntexturedNode( tinygltf::Model& model, const CLAYER_TRIANGLE_CONTAINER* triangles ) {
    if (triangles->GetVertexSize() == 0) {
        // Don't do anything if the layer is empty
        return;
    }

    bool add_normals = false;
    if (triangles->GetNormalsSize()) {
        add_normals = true;
    }

    size_t length = triangles->GetVertexSize();
    size_t length_bytes = length * sizeof(SFVEC3F);

    int node_index = model.nodes.size();

    model.nodes[0].children.push_back(node_index);

    int mesh_index = model.meshes.size();

    tinygltf::Node node = tinygltf::Node();
    node.mesh = mesh_index;
    model.nodes.push_back(node);

    int vertex_buf_view_index = model.bufferViews.size();
    int normal_buf_view_index = vertex_buf_view_index + 1;
    int vertex_accessor_index = model.accessors.size();
    int normal_accessor_index = vertex_accessor_index + 1;

    tinygltf::Mesh mesh = tinygltf::Mesh();
    tinygltf::Primitive primitive = tinygltf::Primitive();
    primitive.attributes.insert(std::pair<std::string, int>("POSITION", vertex_accessor_index));
    if (add_normals) {
        primitive.attributes.insert(std::pair<std::string, int>("NORMAL", normal_accessor_index));
    }
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    mesh.primitives.push_back(primitive);
    model.meshes.push_back(mesh);

    int vertex_buffer_byte_offset = model.buffers[0].data.size();
    int normal_buffer_byte_offset = vertex_buffer_byte_offset + length_bytes;

    const unsigned char* mid_vertices_data = (const unsigned char*) triangles->GetVertexPointer();
    std::copy(&mid_vertices_data[0], &mid_vertices_data[length_bytes], std::back_inserter(model.buffers[0].data));
    if (add_normals) {
        const unsigned char* mid_normals_data = (const unsigned char*) triangles->GetNormalsPointer();
        std::copy(&mid_normals_data[0], &mid_normals_data[length_bytes], std::back_inserter(model.buffers[0].data));
    }

    tinygltf::BufferView vertices_view = tinygltf::BufferView();
    vertices_view.buffer = 0;
    vertices_view.byteOffset = vertex_buffer_byte_offset;
    vertices_view.byteLength = length_bytes;
    vertices_view.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(vertices_view);
    if (add_normals) {
        tinygltf::BufferView normals_view = tinygltf::BufferView();
        normals_view.buffer = 0;
        normals_view.byteOffset = normal_buffer_byte_offset;
        normals_view.byteLength = length_bytes;
        normals_view.target = TINYGLTF_TARGET_ARRAY_BUFFER;
        model.bufferViews.push_back(normals_view);
    }

    tinygltf::Accessor vertices_accessor = tinygltf::Accessor();
    vertices_accessor.bufferView = vertex_buf_view_index;
    vertices_accessor.byteOffset = 0;
    vertices_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    vertices_accessor.count = length;
    vertices_accessor.type = TINYGLTF_TYPE_VEC3;
    vertices_accessor.minValues = vec3_elementwise_min(triangles->GetVertexPointer(), length);
    vertices_accessor.maxValues = vec3_elementwise_max(triangles->GetVertexPointer(), length);
    model.accessors.push_back(vertices_accessor);
    if (add_normals) {
        tinygltf::Accessor normals_accessor = tinygltf::Accessor();
        normals_accessor.bufferView = normal_buf_view_index;
        normals_accessor.byteOffset = 0;
        normals_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        normals_accessor.count = length;
        normals_accessor.type = TINYGLTF_TYPE_VEC3;
        normals_accessor.minValues = vec3_elementwise_min(triangles->GetNormalsPointer(), length);
        normals_accessor.maxValues = vec3_elementwise_max(triangles->GetNormalsPointer(), length);
        model.accessors.push_back(normals_accessor);
    }
}

/**
 * @brief PushTexturedNode - Adds a new textured mesh node to the GLTF model
 * from the given triangle container.
 */
void PushTexturedNode( tinygltf::Model& model, const CLAYER_TRIANGLE_CONTAINER* triangles, int material_index ) {
    if (triangles->GetVertexSize() == 0) {
        // Don't do anything if the layer is empty
        return;
    }

    size_t length = triangles->GetVertexSize();
    size_t length_bytes = length * sizeof(SFVEC3F);

    SFVEC2F *uv_data = new SFVEC2F[length];
    for( unsigned int i = 0; i < length; i += 3 ) {
        uv_data[i + 0] = SFVEC2F( 1.0f, 0.0f );
        uv_data[i + 1] = SFVEC2F( 0.0f, 1.0f );
        uv_data[i + 2] = SFVEC2F( 0.0f, 0.0f );
    }

    size_t uv_length = length;
    size_t uv_length_bytes = uv_length * sizeof(SFVEC2F);

    int node_index = model.nodes.size();

    model.nodes[0].children.push_back(node_index);

    int mesh_index = model.meshes.size();

    tinygltf::Node node = tinygltf::Node();
    node.mesh = mesh_index;
    model.nodes.push_back(node);

    int vertex_buf_view_index = model.bufferViews.size();
    int texcoord_buf_view_index = vertex_buf_view_index + 1;
    int vertex_accessor_index = model.accessors.size();
    int texcoord_accessor_index = vertex_accessor_index + 1;

    tinygltf::Mesh mesh = tinygltf::Mesh();
    tinygltf::Primitive primitive = tinygltf::Primitive();
    primitive.attributes.insert(std::pair<std::string, int>("POSITION", vertex_accessor_index));
    primitive.attributes.insert(std::pair<std::string, int>("TEXCOORD_0", texcoord_accessor_index));
    primitive.material = material_index;
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    mesh.primitives.push_back(primitive);
    model.meshes.push_back(mesh);

    int vertex_buffer_byte_offset = model.buffers[0].data.size();
    int texcoord_buffer_byte_offset = vertex_buffer_byte_offset + length_bytes;

    const unsigned char* vertex_data = (const unsigned char*) triangles->GetVertexPointer();
    std::copy(&vertex_data[0], &vertex_data[length_bytes], std::back_inserter(model.buffers[0].data));
    const unsigned char* uv_data_bytes = (const unsigned char*) uv_data;
    std::copy(&uv_data_bytes[0], &uv_data_bytes[uv_length_bytes], std::back_inserter(model.buffers[0].data));

    tinygltf::BufferView vertices_view = tinygltf::BufferView();
    vertices_view.buffer = 0;
    vertices_view.byteOffset = vertex_buffer_byte_offset;
    vertices_view.byteLength = length_bytes;
    vertices_view.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(vertices_view);
    tinygltf::BufferView uv_view = tinygltf::BufferView();
    uv_view.buffer = 0;
    uv_view.byteOffset = texcoord_buffer_byte_offset;
    uv_view.byteLength = uv_length_bytes;
    uv_view.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(uv_view);

    tinygltf::Accessor vertices_accessor = tinygltf::Accessor();
    vertices_accessor.bufferView = vertex_buf_view_index;
    vertices_accessor.byteOffset = 0;
    vertices_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    vertices_accessor.count = length;
    vertices_accessor.type = TINYGLTF_TYPE_VEC3;
    vertices_accessor.minValues = vec3_elementwise_min(triangles->GetVertexPointer(), length);
    vertices_accessor.maxValues = vec3_elementwise_max(triangles->GetVertexPointer(), length);
    model.accessors.push_back(vertices_accessor);
    tinygltf::Accessor uv_accessor = tinygltf::Accessor();
    uv_accessor.bufferView = texcoord_buf_view_index;
    uv_accessor.byteOffset = 0;
    uv_accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    uv_accessor.count = uv_length;
    uv_accessor.type = TINYGLTF_TYPE_VEC2;
    uv_accessor.minValues = std::vector<double>{0., 0.};
    uv_accessor.maxValues = std::vector<double>{1., 1.};
    model.accessors.push_back(uv_accessor);
}

/**
 * @brief AddCircleTexture - Adds a circular image to the GLTF model for use as
 * an alpha mask texture.
 */
int AddCircleTexture( tinygltf::Model& model ) {
    std::vector<unsigned char> circle_tex_data = CircleImage();

    int texture_byte_offset = model.buffers[0].data.size();

    std::copy(circle_tex_data.begin(), circle_tex_data.end(), std::back_inserter(model.buffers[0].data));

    int buffer_view_index = model.bufferViews.size();

    tinygltf::BufferView circle_tex_bufview = tinygltf::BufferView();
    circle_tex_bufview.buffer = 0;
    circle_tex_bufview.byteOffset = texture_byte_offset;
    circle_tex_bufview.byteLength = circle_tex_data.size();
    circle_tex_bufview.target = 0;
    model.bufferViews.push_back(circle_tex_bufview);

    int material_number = model.materials.size();

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
    circle_image.bufferView = buffer_view_index;
    circle_image.mimeType = "image/png";
    model.images.push_back(circle_image);
    tinygltf::Sampler circle_tex_sampler = tinygltf::Sampler();
    circle_tex_sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    circle_tex_sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
    circle_tex_sampler.wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
    circle_tex_sampler.wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
    model.samplers.push_back(circle_tex_sampler);

    return material_number;
}

/**
 * @brief WriteGLTF - Writes the geometry from the given PCB layer into a GLB
 * file of the given name.
 */
void WriteGLTF( const CLAYER_TRIANGLES* geometry, const std::string& gltf_name ) {
    tinygltf::Model model = tinygltf::Model();
    model.asset.version = "2.0";
    model.scenes.push_back(tinygltf::Scene());
    model.defaultScene = 0;
    model.buffers.push_back(tinygltf::Buffer());
    model.nodes.push_back(tinygltf::Node());
    model.scenes[0].nodes.push_back(0);

    PushUntexturedNode(model, geometry->m_layer_top_triangles);
    PushUntexturedNode(model, geometry->m_layer_middle_contourns_quads);
    PushUntexturedNode(model, geometry->m_layer_bot_triangles);

    if(geometry->m_layer_top_segment_ends->GetVertexSize() || geometry->m_layer_bot_segment_ends->GetVertexSize()) {
        int circle_material = AddCircleTexture(model);
        PushTexturedNode(model, geometry->m_layer_top_segment_ends, circle_material);
        PushTexturedNode(model, geometry->m_layer_bot_segment_ends, circle_material);
    }

    tinygltf::TinyGLTF loader;
    loader.WriteGltfSceneToFile(&model, gltf_name + ".glb", false, true, false, true);
}
