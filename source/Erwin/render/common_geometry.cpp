#include "render/common_geometry.h"
#include "render/renderer.h"
#include "core/core.h"
#include <kibble/logger/logger.h>
#include "asset/procedural_geometry.h"

#include <vector>
#include <map>
#include <cstdio>

namespace erwin
{

struct NamedMesh
{
	Mesh mesh;
	std::string name;
};

struct CommonGeometryStorage
{
	std::map<hash_t, NamedMesh> meshes_;
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

	IndexBufferHandle IBO = Renderer::create_index_buffer(idata.data(), uint32_t(idata.size()), primitive);
	VertexBufferHandle VBO = Renderer::create_vertex_buffer(layout_handle, vdata.data(), uint32_t(vdata.size()), UsagePattern::Static);
	VertexArrayHandle VAO = Renderer::create_vertex_array(VBO, IBO);

	s_storage.meshes_.insert({hname, {{VAO, layout_handle, dims, hname, true}, name}});
	return VAO;
}

void CommonGeometry::init()
{
	KLOGN("render") << "Creating procedural common geometry" << std::endl;

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
	// PBR icosphere
	make_geometry("icosphere_pbr", PBR_VBL, &pg::make_icosphere);

	// Renderer::flush();
}

void CommonGeometry::shutdown()
{
	for(auto&& [hname, nmesh]: s_storage.meshes_)
		Renderer::destroy(nmesh.mesh.VAO);
	s_storage.meshes_.clear();
}

const Mesh& CommonGeometry::get_mesh(hash_t name)
{
	auto it = s_storage.meshes_.find(name);
	K_ASSERT(it!=s_storage.meshes_.end(), "[CommonGeometry] Cannot find mesh at that name.");
	return it->second.mesh;
}

void CommonGeometry::visit_meshes(MeshVisitor visit)
{
	for(auto&& [hname, nmesh]: s_storage.meshes_)
	{
		if(visit(nmesh.mesh, nmesh.name))
			break;
	}
}


} // namespace erwin