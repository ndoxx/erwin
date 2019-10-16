#pragma once

#include "render/buffer.h"
#include "render/shader.h"
#include "render/render_state.h" // For access to enums
#include "render/query_timer.h"
#include "render/camera_2d.h"
#include "render/texture_atlas.h"
#include "glm/glm.hpp"

namespace erwin
{

struct SceneData
{
	glm::mat4 view_projection_matrix;
	glm::mat4 view_matrix;
	FrustumSides frustum_sides;
	WRef<Texture2D> texture;
};

enum PPFlags: uint8_t
{
	PP_EN_CHROMATIC_ABERRATION = 1,
	PP_EN_EXPOSURE_TONE_MAPPING = 2,
	PP_EN_VIBRANCE = 4,
	PP_EN_SATURATION = 8,
	PP_EN_CONTRAST = 16,
	PP_EN_GAMMA = 32
};

// #pragma pack(push,1)
struct PostProcData
{
	void set_flag_enabled(PPFlags flag, bool value) { if(value) set_flag(flag); else clear_flag(flag); }
	void set_flag(PPFlags flag)   { flags |= flag; }
	void clear_flag(PPFlags flag) { flags &= ~flag; }
	bool get_flag(PPFlags flag)   { return (flags & flag); }

	glm::vec4 vib_balance = glm::vec4(0.5f); // Vibrance
	glm::vec4 cor_gamma = glm::vec4(1.f);    // Color correction
	float ca_shift = 0.f;                    // Chromatic aberration
	float ca_strength = 0.f;                 // Chromatic aberration
	float tm_exposure = 2.718f;              // Exposure tone mapping
	float vib_strength = 0.f;                // Vibrance
	float cor_saturation = 1.f;              // Color correction
	float cor_contrast = 1.f;                // Color correction
	
	// Filled in by renderer
	glm::vec2 fb_size;                       // Framebuffer size
	
	uint32_t flags = 0;						 // Flags to enable/disable post-processing features
};
// #pragma pack(pop)

class Renderer2D
{
public:
	Renderer2D(uint32_t max_batch_count=8192);
	~Renderer2D();

	// Reset renderer flags for next submissions
	void begin_scene(const PassState& render_state, const OrthographicCamera2D& camera, WRef<Texture2D> texture, const PostProcData& pp_data);
	// Upload last batch and flush all batches
	void end_scene();
	// Request the renderer implementation to push a quad to current batch
	void draw_quad(const glm::vec2& position, 
				   const glm::vec2& scale,
				   const glm::vec4& uvs);
	// Modify maximum batch size
	void set_batch_size(uint32_t value);

private:
	void create_batch();
	void upload_batch();
	void remove_unused_batches(uint32_t index);

private:
	struct InstanceData // Need correct alignment for SSBO data
	{
		glm::vec2 offset;
		glm::vec2 scale;
		glm::vec4 uvs;
	};

	uint32_t max_batch_count_;   // max number of quads in a batch
	uint32_t current_batch_;
	uint32_t current_batch_count_;
	PassState render_state_;
	SceneData scene_data_;
	PostProcData post_proc_data_;
	WRef<UniformBuffer> mat_ubo_;
	WRef<UniformBuffer> pp_ubo_;
	WRef<VertexArray> quad_va_;
	std::vector<WRef<ShaderStorageBuffer>> batches_;
	std::vector<InstanceData> instance_data_;
	std::vector<uint16_t> batch_ttl_;
};

} // namespace erwin