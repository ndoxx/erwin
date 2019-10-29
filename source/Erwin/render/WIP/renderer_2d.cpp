#include "render/WIP/renderer_2d.h"
#include "glm/gtc/matrix_transform.hpp"

namespace erwin
{
namespace WIP
{

struct Renderer2DStorage
{
	IndexBufferHandle ibo_handle;
	VertexBufferLayoutHandle vbl_handle;
	VertexBufferHandle vbo_handle;
	VertexArrayHandle va_handle;
	ShaderHandle shader_handle;
	UniformBufferHandle ubo_handle;

	glm::mat4 view_projection_matrix;
	glm::mat4 view_matrix;
	FrustumSides frustum_sides;
};

static Renderer2DStorage storage;

void Renderer2D::init()
{
	MainRenderer::init();

	float sq_vdata[20] = 
	{
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f,   0.0f, 1.0f
	};
	uint32_t index_data[6] = { 0, 1, 2, 2, 3, 0 };

	auto& rq = MainRenderer::get_queue(MainRenderer::Resource);
	storage.shader_handle = rq.create_shader(filesystem::get_asset_dir() / "shaders/color_shader.glsl", "color_shader");
	storage.ibo_handle = rq.create_index_buffer(index_data, 6, DrawPrimitive::Triangles);
	storage.vbl_handle = rq.create_vertex_buffer_layout({
			    				 			    	{"a_position"_h, ShaderDataType::Vec3},
								 			    	{"a_uv"_h,       ShaderDataType::Vec2},
								 			    });
	storage.vbo_handle = rq.create_vertex_buffer(storage.vbl_handle, sq_vdata, 20, DrawMode::Static);
	storage.va_handle = rq.create_vertex_array(storage.vbo_handle, storage.ibo_handle);
	storage.ubo_handle = rq.create_uniform_buffer("matrices", nullptr, sizeof(glm::mat4), DrawMode::Dynamic);
}

void Renderer2D::shutdown()
{
	auto& rq = MainRenderer::get_queue(MainRenderer::Resource);
	rq.destroy_uniform_buffer(storage.ubo_handle);
	rq.destroy_vertex_array(storage.va_handle);
	rq.destroy_vertex_buffer(storage.vbo_handle);
	rq.destroy_vertex_buffer_layout(storage.vbl_handle);
	rq.destroy_index_buffer(storage.ibo_handle);
	rq.destroy_shader(storage.shader_handle);

	MainRenderer::flush();
}

void Renderer2D::begin_pass(const PassState& state, const OrthographicCamera2D& camera)
{
	auto& rq = MainRenderer::get_queue(MainRenderer::Resource);
	rq.set_state(state);

	// Set scene data
	storage.view_projection_matrix = camera.get_view_projection_matrix();
	storage.view_matrix = camera.get_view_matrix();
	storage.frustum_sides = camera.get_frustum_sides();
}

void Renderer2D::end_pass()
{

}

void Renderer2D::draw_quad(const glm::vec2& position, const glm::vec2& scale, const glm::vec4& uvs, hash_t atlas)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(position,0.f)) * glm::scale(glm::mat4(1.f), {scale.x, scale.y, 1.f});
	glm::mat4 MVP = storage.view_projection_matrix * transform;

	auto& rq = MainRenderer::get_queue(MainRenderer::Resource);
	// TODO: batch quads like in the old 2D renderer
	DrawCall dc(DrawCall::Indexed, storage.shader_handle, storage.va_handle);
	dc.set_per_instance_UBO(storage.ubo_handle, &MVP, sizeof(glm::mat4));
	rq.submit(dc);

}
/*
void Renderer2D::draw_quad(const glm::vec3& position, const glm::vec2& scale, const glm::vec4& uvs, hash_t atlas)
{

}
*/

} // namespace WIP
} // namespace erwin