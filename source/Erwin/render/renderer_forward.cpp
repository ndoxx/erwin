#include "render/renderer_forward.h"
#include "render/common_geometry.h"
#include "render/main_renderer.h"
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
	ShaderHandle forward_shader;
	UniformBufferHandle instance_ubo;
	UniformBufferHandle pass_ubo;

	PassUBOData pass_ubo_data;

	glm::mat4 view_matrix;
	FrustumPlanes frustum_planes;
	uint64_t pass_state;

	uint32_t num_draw_calls; // stats
	uint8_t layer_id;
};
static ForwardRenderer3DStorage storage;

void ForwardRenderer::init()
{
    W_PROFILE_FUNCTION()

    FramebufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    FramebufferPool::create_framebuffer("fb_forward"_h, make_scope<FbRatioConstraint>(), layout, true);

	storage.num_draw_calls = 0;

	storage.forward_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/forward_shader.glsl", "forward_shader");
	// storage.forward_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/forward_shader.spv", "forward_shader");
	storage.instance_ubo = MainRenderer::create_uniform_buffer("instance_data", nullptr, sizeof(InstanceData), DrawMode::Dynamic);
	storage.pass_ubo = MainRenderer::create_uniform_buffer("pass_data", nullptr, sizeof(PassUBOData), DrawMode::Dynamic);
	
	MainRenderer::shader_attach_uniform_buffer(storage.forward_shader, storage.instance_ubo);
	// MainRenderer::shader_attach_uniform_buffer(storage.forward_shader, storage.pass_ubo);
}

void ForwardRenderer::shutdown()
{
	MainRenderer::destroy(storage.forward_shader);
	MainRenderer::destroy(storage.instance_ubo);
}

void ForwardRenderer::begin_pass(const PerspectiveCamera3D& camera, bool transparent, uint8_t layer_id)
{
    W_PROFILE_FUNCTION()

	PassState state;
	state.render_target = FramebufferPool::get_framebuffer("fb_forward"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = transparent ? BlendState::Alpha : BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;
	state.rasterizer_state.clear_color = glm::vec4(0.2f,0.2f,0.2f,0.f);

	// Reset stats
	storage.num_draw_calls = 0;

	// Pass state
	storage.pass_state = state.encode();
	storage.layer_id = layer_id;
	MainRenderer::get_queue("Forward"_h).set_clear_color(state.rasterizer_state.clear_color); // TMP

	// Set scene data
	storage.pass_ubo_data.view_projection_matrix = camera.get_view_projection_matrix();
	storage.view_matrix = camera.get_view_matrix();
	storage.frustum_planes = camera.get_frustum_planes();

	MainRenderer::update_uniform_buffer(storage.pass_ubo, &storage.pass_ubo_data, sizeof(PassUBOData));
}

void ForwardRenderer::end_pass()
{

}

void ForwardRenderer::draw_colored_cube(const glm::vec3& position, float scale, const glm::vec4& tint)
{
	InstanceData instance_data;
	instance_data.tint = tint;
	instance_data.mvp = storage.pass_ubo_data.view_projection_matrix 
				      * glm::translate(glm::mat4(1.f), position)
				      * glm::scale(glm::mat4(1.f), glm::vec3(scale,scale,scale));

	auto& q_forward = MainRenderer::get_queue("Forward"_h);
	DrawCall dc(q_forward, DrawCall::Indexed, storage.forward_shader, CommonGeometry::get_vertex_array("cube"_h));
	dc.set_state(storage.pass_state);
	dc.set_per_instance_UBO(storage.instance_ubo, (void*)&instance_data, sizeof(InstanceData));
	dc.set_key_depth(position.z, storage.layer_id);
	dc.submit();

	++storage.num_draw_calls;
}

uint32_t ForwardRenderer::get_draw_call_count()
{
	return storage.num_draw_calls;
}


} // namespace erwin