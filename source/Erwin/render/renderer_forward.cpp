#include "render/renderer_forward.h"
#include "glm/gtc/matrix_transform.hpp"

namespace erwin
{

struct PassUBOData
{
	glm::mat4 view_projection_matrix;
};

struct InstanceData
{
	glm::mat4 mvp;
	glm::vec4 tint;
};

struct ForwardRenderer3DStorage
{
	IndexBufferHandle cube_ibo;
	VertexBufferLayoutHandle cube_vbl;
	VertexBufferHandle cube_vbo;
	VertexArrayHandle cube_va;
	ShaderHandle forward_shader;
	UniformBufferHandle instance_ubo;
	UniformBufferHandle pass_ubo;

	PassUBOData pass_ubo_data;

	glm::mat4 view_matrix;
	FrustumPlanes frustum_planes;
	uint64_t state_flags;

	uint32_t num_draw_calls; // stats
	uint8_t layer_id;
};
static ForwardRenderer3DStorage storage;

void ForwardRenderer::init()
{
    W_PROFILE_FUNCTION()

	storage.num_draw_calls = 0;

	float cube_vdata[24*3] = 
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
	uint32_t index_data[12*3] =
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

	storage.forward_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/forward_shader.glsl", "forward_shader");
	// storage.forward_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/forward_shader.spv", "forward_shader");

	storage.cube_ibo = MainRenderer::create_index_buffer(index_data, 12*3, DrawPrimitive::Triangles);
	storage.cube_vbl = MainRenderer::create_vertex_buffer_layout({
			    				 			    	{"a_position"_h, ShaderDataType::Vec3}
								 			    	});
	storage.cube_vbo = MainRenderer::create_vertex_buffer(storage.cube_vbl, cube_vdata, 24*3, DrawMode::Static);
	storage.cube_va = MainRenderer::create_vertex_array(storage.cube_vbo, storage.cube_ibo);
	storage.instance_ubo = MainRenderer::create_uniform_buffer("instance_data", nullptr, sizeof(InstanceData), DrawMode::Dynamic);
	storage.pass_ubo = MainRenderer::create_uniform_buffer("pass_data", nullptr, sizeof(PassUBOData), DrawMode::Dynamic);
	
	MainRenderer::shader_attach_uniform_buffer(storage.forward_shader, storage.instance_ubo);
	MainRenderer::shader_attach_uniform_buffer(storage.forward_shader, storage.pass_ubo);
}

void ForwardRenderer::shutdown()
{
	MainRenderer::destroy(storage.forward_shader);
	MainRenderer::destroy(storage.instance_ubo);
	MainRenderer::destroy(storage.cube_va);
	MainRenderer::destroy(storage.cube_vbo);
	MainRenderer::destroy(storage.cube_vbl);
	MainRenderer::destroy(storage.cube_ibo);
}

void ForwardRenderer::begin_pass(const PassState& state, const PerspectiveCamera3D& camera, uint8_t layer_id)
{
    W_PROFILE_FUNCTION()

	// Reset stats
	storage.num_draw_calls = 0;

	// Pass state
	storage.state_flags = state.encode();
	storage.layer_id = layer_id;

	// Set scene data
	storage.pass_ubo_data.view_projection_matrix = camera.get_view_projection_matrix();
	storage.view_matrix = camera.get_view_matrix();
	storage.frustum_planes = camera.get_frustum_planes();

	MainRenderer::update_uniform_buffer(storage.pass_ubo, &storage.pass_ubo_data, sizeof(PassUBOData));
}

void ForwardRenderer::end_pass()
{
    W_PROFILE_FUNCTION()
    
	flush();
}

void ForwardRenderer::draw_colored_cube(const glm::vec3& position, float scale, const glm::vec4& tint)
{
	InstanceData instance_data;
	instance_data.tint = tint;
	instance_data.mvp = storage.pass_ubo_data.view_projection_matrix 
				      * glm::translate(glm::mat4(1.f), position)
				      * glm::scale(glm::mat4(1.f), glm::vec3(scale,scale,scale));

	auto& q_forward = MainRenderer::get_queue("Forward"_h);
	DrawCall dc(q_forward, DrawCall::Indexed, storage.forward_shader, storage.cube_va);
	dc.set_state(storage.state_flags);
	dc.set_per_instance_UBO(storage.instance_ubo, (void*)&instance_data, sizeof(InstanceData));
	dc.set_key_depth(position.z, storage.layer_id);
	dc.submit();

	++storage.num_draw_calls;
}

void ForwardRenderer::flush()
{

}

uint32_t ForwardRenderer::get_draw_call_count()
{
	return storage.num_draw_calls;
}


} // namespace erwin