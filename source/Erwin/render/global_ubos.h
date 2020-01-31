#pragma once

#include "glm/glm.hpp"
#include "render/handles.h"

namespace erwin
{
class PerspectiveCamera3D;
class ComponentDirectionalLight;

namespace gu
{

struct FrameData
{
	glm::mat4 view_matrix;
	glm::mat4 view_projection_matrix;
	glm::vec4 eye_position;
	glm::vec4 camera_params;
	glm::vec4 framebuffer_size; // x,y: framebuffer dimensions in pixels, z: aspect ratio, w: padding
	glm::vec4 proj_params;
	
	glm::vec4 light_position;
	glm::vec4 light_color;
	glm::vec4 light_ambient_color;
	float light_ambient_strength;
};

struct TransformData
{
	glm::mat4 m;
	glm::mat4 mv;
	glm::mat4 mvp;
};

extern void init();
extern void shutdown();

extern void update_frame_data(const PerspectiveCamera3D& camera, const ComponentDirectionalLight& dir_light);
extern UniformBufferHandle get_frame_ubo();
extern const FrameData& get_frame_data();

extern UniformBufferHandle get_transform_ubo();

} // namespace gu
} // namespace erwin