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
namespace WIP // Work in progress ;)
{

struct RenderStats
{
	float render_time = 0.f;
	uint32_t batches = 0;
};

struct SceneData
{
	glm::mat4 view_projection_matrix;
	glm::mat4 view_matrix;
	FrustumSides frustum_sides;
	WRef<Texture2D> texture;
};

class Renderer2D
{
public:
	Renderer2D(uint32_t max_batch_count=8192);
	~Renderer2D();

	// Reset renderer flags for next submissions
	void begin_scene(const PassState& render_state, const OrthographicCamera2D& camera, WRef<Texture2D> texture);
	// Upload last batch and flush all batches
	void end_scene();
	// Request the renderer implementation to push a quad to current batch
	void draw_quad(const glm::vec2& position, 
				   const glm::vec2& scale,
				   const glm::vec4& uvs);
	// Modify maximum batch size
	void set_batch_size(uint32_t value);

	// Enable/Disable GPU query timer profiling
	inline void set_profiling_enabled(bool value = true) { profiling_enabled_ = value; }
	inline const RenderStats& get_stats() const          { return stats_; }

private:
	inline void reset_stats() { stats_.render_time = 0.f; stats_.batches = 0; }

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
	RenderStats stats_;
	PassState render_state_;
	SceneData scene_data_;
	WRef<UniformBuffer> mat_ubo_;
	WRef<VertexArray> quad_va_;
	std::vector<WRef<ShaderStorageBuffer>> batches_;
	std::vector<InstanceData> instance_data_;
	std::vector<uint16_t> batch_ttl_;
	bool profiling_enabled_ = false;
	WScope<QueryTimer> query_timer_;
};

} // namespace WIP
} // namespace erwin