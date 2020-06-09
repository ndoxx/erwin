#include "asset/mesh_loader.h"
#include "render/renderer.h"

namespace erwin
{

AssetMetaData MeshLoader::build_meta_data(const fs::path& file_path)
{
    W_ASSERT_FMT(fs::exists(file_path), "File does not exist: %s", file_path.string().c_str());
    W_ASSERT(!file_path.extension().string().compare(".wesh"), "Invalid input file.");

    return {file_path, AssetMetaData::AssetType::MeshWESH};
}

wesh::WeshDescriptor MeshLoader::load_from_file(const AssetMetaData& meta_data)
{
    DLOG("asset", 1) << "Loading WESH file:" << std::endl;
    DLOGI << WCC('p') << meta_data.file_path << std::endl;

    return wesh::read(meta_data.file_path);
}

Mesh MeshLoader::upload(const wesh::WeshDescriptor& descriptor)
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

    return {VAO, PBR_VBL, descriptor.extent, {}};
}

void MeshLoader::destroy(Mesh& mesh) { Renderer::destroy(mesh.VAO); }

} // namespace erwin