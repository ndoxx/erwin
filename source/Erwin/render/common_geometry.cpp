#include "render/common_geometry.h"
#include "render/renderer.h"
#include "core/core.h"
#include "debug/logger.h"
#include "asset/procedural_geometry.h"

#include <vector>
#include <map>
#include <cstdio>

namespace erwin
{

struct CommonGeometryStorage
{
	std::map<hash_t, MeshStub> meshes_;
};
static CommonGeometryStorage s_storage;

using geom_func_ptr_t = Extent (*)(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, pg::Parameters* params);
static VertexArrayHandle make_geometry(const std::string& name, VertexBufferLayoutHandle layout_handle, geom_func_ptr_t geom, DrawPrimitive primitive=DrawPrimitive::Triangles)
{
	std::vector<float> vdata;
	std::vector<uint32_t> idata;

	hash_t hname = H_(name.c_str());
	const auto& layout = Renderer::get_vertex_buffer_layout(layout_handle);
	Extent dims = (*geom)(layout, vdata, idata, nullptr);

	IndexBufferHandle IBO = Renderer::create_index_buffer(idata.data(), idata.size(), primitive);
	VertexBufferHandle VBO = Renderer::create_vertex_buffer(layout_handle, vdata.data(), vdata.size(), UsagePattern::Static);
	VertexArrayHandle VAO = Renderer::create_vertex_array(VBO, IBO);

	MeshStub mesh {VAO, layout_handle, dims};
	snprintf(mesh.name, 32, "%s", name.c_str());

	s_storage.meshes_.insert({hname, mesh});
	return VAO;
}

void CommonGeometry::init()
{
	DLOGN("render") << "Creating procedural common geometry" << std::endl;

	// Buffer layouts are created immediately, so their data can be queried here and now
	VertexBufferLayoutHandle pos_VBL    = Renderer::create_vertex_buffer_layout({
			    						  	{"a_position"_h, ShaderDataType::Vec3},
										  });
	VertexBufferLayoutHandle pos_uv_VBL = Renderer::create_vertex_buffer_layout({
			    						  	{"a_position"_h, ShaderDataType::Vec3},
										  	{"a_uv"_h,       ShaderDataType::Vec2},
										  });
	VertexBufferLayoutHandle PBR_VBL    = Renderer::create_vertex_buffer_layout({
			    						  	{"a_position"_h, ShaderDataType::Vec3},
			    						  	{"a_normal"_h,   ShaderDataType::Vec3},
			    						  	{"a_tangent"_h,  ShaderDataType::Vec3},
										  	{"a_uv"_h,       ShaderDataType::Vec2},
										  });

	// Screen-space textured quad
	make_geometry("quad", pos_uv_VBL, &pg::make_plane);
	// Cube
	make_geometry("cube", pos_VBL, &pg::make_cube);
	// Cube lines
	make_geometry("cube_lines", pos_VBL, &pg::make_cube_lines, DrawPrimitive::Lines);
	// Origin lines
	make_geometry("origin_lines", pos_VBL, &pg::make_origin, DrawPrimitive::Lines);
	// UV Cube
	make_geometry("cube_uv", pos_uv_VBL, &pg::make_cube);
	// PBR Cube
	make_geometry("cube_pbr", PBR_VBL, &pg::make_cube);
	// PBR Icosahedron
	make_geometry("icosahedron_pbr", PBR_VBL, &pg::make_icosahedron);

	// Renderer::flush();
}

void CommonGeometry::shutdown()
{

}

VertexArrayHandle CommonGeometry::get_vertex_array(hash_t name)
{
	auto it = s_storage.meshes_.find(name);
	W_ASSERT(it!=s_storage.meshes_.end(), "[CommonGeometry] Cannot find mesh at that name.");
	return it->second.VAO;
}

const Extent& CommonGeometry::get_extent(hash_t name)
{
	auto it = s_storage.meshes_.find(name);
	W_ASSERT(it!=s_storage.meshes_.end(), "[CommonGeometry] Cannot find mesh at that name.");
	return it->second.extent;
}

void CommonGeometry::visit_meshes(MeshVisitor visit)
{
	for(auto&& [hname, mesh]: s_storage.meshes_)
	{
		if(visit(mesh))
			break;
	}
}


} // namespace erwin