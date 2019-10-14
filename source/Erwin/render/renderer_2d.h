#pragma once

#include "render/buffer.h"
#include "render/shader.h"
#include "render/render_state.h" // For access to enums
#include "render/query_timer.h" // TMP, will be moved to render thread
#include "render/camera_2d.h"
#include "render/texture_atlas.h"


#include "glm/glm.hpp"

namespace erwin
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

// Base class for 2D renderers
class Renderer2D
{
public:
	Renderer2D(uint32_t max_batch_count=8192);
	virtual ~Renderer2D();

	// Reset renderer flags for next submissions
	void begin_scene(const RenderState& render_state, const OrthographicCamera2D& camera, WRef<Texture2D> texture, const PostProcData& pp_data);
	// Upload last batch and flush all batches
	void end_scene();
	// Request the renderer implementation to push a quad to current batch
	void draw_quad(const glm::vec2& position, 
				   const glm::vec2& scale,
				   const glm::vec4& uvs);
	// Modify maximum batch size
	virtual void set_batch_size(uint32_t value) = 0;

	// Enable/Disable GPU query timer profiling
	inline void set_profiling_enabled(bool value = true) { profiling_enabled_ = value; }
	inline const RenderStats& get_stats() const          { return stats_; }

	static ShaderBank shader_bank;

protected:
	// Draw all batches
	virtual void flush() = 0;
	// Push a quad to current batch
	virtual void push_quad(const glm::vec2& position, const glm::vec2& scale, const glm::vec4& uvs) = 0;
	// Send current batch data to render device
	virtual void upload_batch() = 0;
	// Get rid of unused batches starting at index (called after flush())
	virtual void remove_unused_batches(uint32_t index) = 0;
	// Get current number of batches
	virtual uint32_t get_num_batches() = 0;
	// Add a new batch (vertex array) to the list
	virtual void create_batch() = 0;

	inline void reset_stats() { stats_.render_time = 0.f; stats_.batches = 0; }

protected:
	uint32_t max_batch_count_;   // max number of quads in a batch
	uint32_t current_batch_;
	uint32_t current_batch_count_;
	RenderStats stats_;
	RenderState render_state_;
	SceneData scene_data_;
	WRef<UniformBuffer> mat_ubo_;

private:
	std::vector<uint16_t> batch_ttl_;
	bool profiling_enabled_ = false;
	WScope<QueryTimer> query_timer_;
	WRef<VertexArray> screen_va_;
	WRef<UniformBuffer> pp_ubo_;
	PostProcData post_proc_data_;
};

class BatchRenderer2D: public Renderer2D
{
public:
	BatchRenderer2D(uint32_t max_batch_count=8192);
	~BatchRenderer2D() = default;

	virtual void set_batch_size(uint32_t value) override;

protected:
	virtual void flush() override;
	virtual void push_quad(const glm::vec2& position, const glm::vec2& scale, const glm::vec4& uvs) override;
	virtual void upload_batch() override;
	virtual void remove_unused_batches(uint32_t index) override;
	virtual uint32_t get_num_batches() override;
	virtual void create_batch() override;

private:
	std::vector<WRef<VertexArray>> batches_;
	std::vector<float> vertex_list_;
};

class InstanceRenderer2D: public Renderer2D
{
public:
	InstanceRenderer2D(uint32_t max_batch_count=8192);
	~InstanceRenderer2D() = default;

	virtual void set_batch_size(uint32_t value) override;

protected:
	virtual void flush() override;
	virtual void push_quad(const glm::vec2& position, const glm::vec2& scale, const glm::vec4& uvs) override;
	virtual void upload_batch() override;
	virtual void remove_unused_batches(uint32_t index) override;
	virtual uint32_t get_num_batches() override;
	virtual void create_batch() override;

private:
	struct InstanceData // Need correct alignment for SSBO data
	{
		glm::vec2 offset;
		glm::vec2 scale;
		glm::vec4 uvs;
	};

	WRef<VertexArray> quad_va_;
	std::vector<WRef<ShaderStorageBuffer>> batches_;
	std::vector<InstanceData> instance_data_;
};

} // namespace erwin