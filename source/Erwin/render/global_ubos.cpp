#include "render/global_ubos.h"
#include "render/renderer.h"
#include "render/camera_3d.h"
#include "entity/light.h"

namespace erwin
{
namespace gu
{

static struct
{
	UniformBufferHandle frame_ubo;
	UniformBufferHandle transform_ubo;
	FrameData frame_data;
} s_storage;

void init()
{
	s_storage.frame_ubo     = Renderer::create_uniform_buffer("frame_data", nullptr, sizeof(FrameData), UsagePattern::Dynamic);
	s_storage.transform_ubo = Renderer::create_uniform_buffer("transform_data", nullptr, sizeof(TransformData), UsagePattern::Dynamic);
}

void shutdown()
{
	Renderer::destroy(s_storage.transform_ubo);
	Renderer::destroy(s_storage.frame_ubo);
}

void update_frame_data(const PerspectiveCamera3D& camera, const ComponentDirectionalLight& dir_light)
{
	glm::vec2 fb_size = FramebufferPool::get_screen_size();
	float near = camera.get_frustum().near;
	float far  = camera.get_frustum().far;

	s_storage.frame_data.view_matrix = camera.get_view_matrix();
	s_storage.frame_data.view_projection_matrix = camera.get_view_projection_matrix();
	s_storage.frame_data.eye_position = glm::vec4(camera.get_position(), 1.f);
	s_storage.frame_data.camera_params = glm::vec4(near,far,0.f,0.f);
	s_storage.frame_data.framebuffer_size = glm::vec4(fb_size, fb_size.x/fb_size.y, 0.f);
	s_storage.frame_data.proj_params = camera.get_projection_parameters();

	s_storage.frame_data.light_position = glm::vec4(dir_light.position, 0.f);
	s_storage.frame_data.light_color = glm::vec4(dir_light.color, 1.f) * dir_light.brightness;
	s_storage.frame_data.light_ambient_color = glm::vec4(dir_light.ambient_color, 1.f);
	s_storage.frame_data.light_ambient_strength = dir_light.ambient_strength;

	Renderer::update_uniform_buffer(s_storage.frame_ubo, &s_storage.frame_data, sizeof(FrameData));
}

UniformBufferHandle get_frame_ubo()
{
	return s_storage.frame_ubo;
}

const FrameData& get_frame_data()
{
	return s_storage.frame_data;
}

UniformBufferHandle get_transform_ubo()
{
	return s_storage.transform_ubo;
}


} // namespace gu
} // namespace erwin