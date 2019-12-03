#include "render/common_geometry.h"
#include "render/main_renderer.h"
#include "core/core.h"
#include "debug/logger.h"

#include <vector>
#include <map>

namespace erwin
{

struct CommonGeometryStorage
{
	std::map<hash_t, VertexArrayHandle> vertex_arrays_;
};
static CommonGeometryStorage s_storage;

void CommonGeometry::init()
{
	DLOGN("render") << "Creating procedural common geometry" << std::endl;

	// Screen-space textured quad
	{
		float vdata[20] = 
		{
			-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f,   0.0f, 1.0f
		};
		uint32_t idata[6] = { 0, 1, 2, 2, 3, 0 };

		IndexBufferHandle IBO = MainRenderer::create_index_buffer(idata, 6, DrawPrimitive::Triangles);
		VertexBufferLayoutHandle VBL = MainRenderer::create_vertex_buffer_layout({
				    					{"a_position"_h, ShaderDataType::Vec3},
										{"a_uv"_h,       ShaderDataType::Vec2},
									});
		VertexBufferHandle VBO = MainRenderer::create_vertex_buffer(VBL, vdata, 20, DrawMode::Static);
		VertexArrayHandle VAO = MainRenderer::create_vertex_array(VBO, IBO);

		s_storage.vertex_arrays_.insert(std::make_pair("screen_quad"_h, VAO));
	}

	// Cube
	{
		float vdata[24*3] = 
		{
			 0.5f, -0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f,  0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f, -0.5f
		};
		uint32_t idata[12*3] =
		{
			0,  1,  2,
			0,  2,  3,
			4,  5,  6,
			4,  6,  7,
			8,  9,  10,
			8,  10, 11,
			12, 13, 14,
			12, 14, 15,
			16, 17, 18,
			16, 18, 19,
			20, 21, 22,
			20, 22, 23
		};

		IndexBufferHandle IBO = MainRenderer::create_index_buffer(idata, 12*3, DrawPrimitive::Triangles);
		VertexBufferLayoutHandle VBL = MainRenderer::create_vertex_buffer_layout({
				    				   {"a_position"_h, ShaderDataType::Vec3}
									  });
		VertexBufferHandle VBO = MainRenderer::create_vertex_buffer(VBL, vdata, 24*3, DrawMode::Static);
		VertexArrayHandle VAO = MainRenderer::create_vertex_array(VBO, IBO);

		s_storage.vertex_arrays_.insert(std::make_pair("cube"_h, VAO));
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