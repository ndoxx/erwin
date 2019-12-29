#include "render/common_geometry.h"
#include "render/main_renderer.h"
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
};
static CommonGeometryStorage s_storage;

static void make_geometry(hash_t hname, VertexBufferLayoutHandle layout, const std::vector<float>& vdata, const std::vector<uint32_t>& idata)
{
	IndexBufferHandle IBO = MainRenderer::create_index_buffer(idata.data(), idata.size(), DrawPrimitive::Triangles);
	VertexBufferHandle VBO = MainRenderer::create_vertex_buffer(layout, vdata.data(), vdata.size(), DrawMode::Static);
	VertexArrayHandle VAO = MainRenderer::create_vertex_array(VBO, IBO);

	s_storage.vertex_arrays_.insert(std::make_pair(hname, VAO));
}

void CommonGeometry::init()
{
	DLOGN("render") << "Creating procedural common geometry" << std::endl;

	// Buffer layouts are created immediately, so their data can be queried here and now
	VertexBufferLayoutHandle pos_VBL    = MainRenderer::create_vertex_buffer_layout({
			    						  	{"a_position"_h, ShaderDataType::Vec3},
										  });
	VertexBufferLayoutHandle pos_uv_VBL = MainRenderer::create_vertex_buffer_layout({
			    						  	{"a_position"_h, ShaderDataType::Vec3},
										  	{"a_uv"_h,       ShaderDataType::Vec2},
										  });
	VertexBufferLayoutHandle PBR_VBL    = MainRenderer::create_vertex_buffer_layout({
			    						  	{"a_position"_h, ShaderDataType::Vec3},
			    						  	{"a_normal"_h,   ShaderDataType::Vec3},
			    						  	{"a_tangent"_h,  ShaderDataType::Vec3},
										  	{"a_uv"_h,       ShaderDataType::Vec2},
										  });

	// Screen-space textured quad
	{
		std::vector<float> vdata;
		std::vector<uint32_t> idata;
		const auto& layout = MainRenderer::get_vertex_buffer_layout(pos_uv_VBL);
		pg::make_plane(layout, vdata, idata);
		make_geometry("screen_quad"_h, pos_uv_VBL, vdata, idata);
	}

	// Cube
	{
		std::vector<float> vdata;
		std::vector<uint32_t> idata;
		const auto& layout = MainRenderer::get_vertex_buffer_layout(pos_VBL);
		pg::make_cube(layout, vdata, idata);
		make_geometry("cube"_h, pos_VBL, vdata, idata);
	}

	// UV Cube
	{
		std::vector<float> vdata;
		std::vector<uint32_t> idata;
		const auto& layout = MainRenderer::get_vertex_buffer_layout(pos_uv_VBL);
		pg::make_cube(layout, vdata, idata);
		make_geometry("cube_uv"_h, pos_uv_VBL, vdata, idata);
	}

	// PBR Cube
	{
		std::vector<float> vdata;
		std::vector<uint32_t> idata;
		const auto& layout = MainRenderer::get_vertex_buffer_layout(PBR_VBL);
		pg::make_cube(layout, vdata, idata);
		make_geometry("cube_pbr"_h, PBR_VBL, vdata, idata);
	}

	MainRenderer::flush();
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

} // namespace erwin