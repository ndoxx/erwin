#include "asset/mesh_loader.h"
#include "render/renderer.h"

namespace erwin
{

AssetMetaData MeshLoader::build_meta_data(const WPath& file_path)
{
    K_ASSERT_FMT(file_path.exists(), "File does not exist: %s", file_path.c_str());
    K_ASSERT(file_path.check_extension(".wesh"_h), "Invalid input file.");

    return {file_path, AssetMetaData::AssetType::MeshWESH};
}

wesh::WeshDescriptor MeshLoader::load_from_file(const AssetMetaData& meta_data)
{
    KLOG("asset", 1) << "Loading WESH file:" << std::endl;
    KLOGI << kb::KS_PATH_ << meta_data.file_path << std::endl;

    return wesh::read(meta_data.file_path);
}

Mesh MeshLoader::upload(const wesh::WeshDescriptor& descriptor, hash_t resource_id)
{
    W_PROFILE_FUNCTION()

    // TODO: Remove VertexBufferLayoutHandle completly, pass instance directly instead
    static VertexBufferLayoutHandle PBR_VBL = Renderer::create_vertex_buffer_layout({
        {"a_position"_h, ShaderDataType::Vec3},
        {"a_normal"_h, ShaderDataType::Vec3},
        {"a_tangent"_h, ShaderDataType::Vec3},
        {"a_uv"_h, ShaderDataType::Vec2},
    });

    IndexBufferHandle IBO = Renderer::create_index_buffer(
        descriptor.index_data.data(), uint32_t(descriptor.index_data.size()), DrawPrimitive::Triangles);
    VertexBufferHandle VBO = Renderer::create_vertex_buffer(
        PBR_VBL, descriptor.vertex_data.data(), uint32_t(descriptor.vertex_data.size()), UsagePattern::Static);
    VertexArrayHandle VAO = Renderer::create_vertex_array(VBO, IBO);

    return {VAO, PBR_VBL, descriptor.extent, resource_id};
}

void MeshLoader::destroy(Mesh& mesh) { Renderer::destroy(mesh.VAO); }

} // namespace erwin