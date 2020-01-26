#include "render/common_geometry.h"
#include "render/renderer.h"
#include "core/core.h"
#include "debug/logger.h"
#include "asset/procedural_geometry.h"

#include <vector>
#include <map>

namespace erwin
{

struct CommonGeometryStorage
{
	std::map<hash_t, VertexArrayHandle> vertex_arrays_;
	std::map<hash_t, Dimensions> dimensions_;
};
static CommonGeometryStorage s_storage;

static void make_geometry(hash_t hname, VertexBufferLayoutHandle layout, const std::vector<float>& vdata, const std::vector<uint32_t>& idata)
{
	IndexBufferHandle IBO = Renderer::create_index_buffer(idata.data(), idata.size(), DrawPrimitive::Triangles);
	VertexBufferHandle VBO = Renderer::create_vertex_buffer(layout, vdata.data(), vdata.size(), UsagePattern::Static);
	VertexArrayHandle VAO = Renderer::create_vertex_array(VBO, IBO);

	s_storage.vertex_arrays_.insert(std::make_pair(hname, VAO));
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
	{
		std::vector<float> vdata;
		std::vector<uint32_t> idata;
		const auto& layout = Renderer::get_vertex_buffer_layout(pos_uv_VBL);
		Dimensions dims = pg::make_plane(layout, vdata, idata);
		hash_t hname = "quad"_h;
		make_geometry(hname, pos_uv_VBL, vdata, idata);
		s_storage.dimensions_.insert(std::make_pair(hname, dims));
	}

	// Cube
	{
		std::vector<float> vdata;
		std::vector<uint32_t> idata;
		const auto& layout = Renderer::get_vertex_buffer_layout(pos_VBL);
		Dimensions dims = pg::make_cube(layout, vdata, idata);
		hash_t hname = "cube"_h;
		make_geometry(hname, pos_VBL, vdata, idata);
		s_storage.dimensions_.insert(std::make_pair(hname, dims));
	}

	// UV Cube
	{
		std::vector<float> vdata;
		std::vector<uint32_t> idata;
		const auto& layout = Renderer::get_vertex_buffer_layout(pos_uv_VBL);
		Dimensions dims = pg::make_cube(layout, vdata, idata);
		hash_t hname = "cube_uv"_h;
		make_geometry(hname, pos_uv_VBL, vdata, idata);
		s_storage.dimensions_.insert(std::make_pair(hname, dims));
	}

	// PBR Cube
	{
		std::vector<float> vdata;
		std::vector<uint32_t> idata;
		const auto& layout = Renderer::get_vertex_buffer_layout(PBR_VBL);
		Dimensions dims = pg::make_cube(layout, vdata, idata);
		hash_t hname = "cube_pbr"_h;
		make_geometry(hname, PBR_VBL, vdata, idata);
		s_storage.dimensions_.insert(std::make_pair(hname, dims));
	}

	// PBR Icosahedron
	{
		std::vector<float> vdata;
		std::vector<uint32_t> idata;
		const auto& layout = Renderer::get_vertex_buffer_layout(PBR_VBL);
		Dimensions dims = pg::make_icosahedron(layout, vdata, idata);
		hash_t hname = "icosahedron_pbr"_h;
		make_geometry(hname, PBR_VBL, vdata, idata);
		s_storage.dimensions_.insert(std::make_pair(hname, dims));
	}

	Renderer::flush();
}

void CommonGeometry::shutdown()
{

}

VertexArrayHandle CommonGeometry::get_vertex_array(hash_t name)
{
	auto it = s_storage.vertex_arrays_.find(name);
	W_ASSERT(it!=s_storage.vertex_arrays_.end(), "[CommonGeometry] Cannot find vertex array at that name.");
	return it->second;
}

const Dimensions& CommonGeometry::get_dimensions(hash_t name)
{
	auto it = s_storage.dimensions_.find(name);
	W_ASSERT(it!=s_storage.dimensions_.end(), "[CommonGeometry] Cannot find dimensions at that name.");
	return it->second;
}

} // namespace erwin