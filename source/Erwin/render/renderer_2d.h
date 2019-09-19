#pragma once

#include "render/buffer.h"
#include "render/shader.h"
#include "render/render_state.h" // For access to enums
#include "render/query_timer.h" // TMP, will be moved to render thread
#include "render/camera_2d.h"

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
};

// Base class for 2D renderers
class Renderer2D
{
public:
	Renderer2D(uint32_t max_batch_count=8192);
	virtual ~Renderer2D();

	// Reset renderer flags for next submissions
	void begin_scene(const OrthographicCamera2D& camera);
	// Upload last batch and flush all batches
	void end_scene();
	// Setup render state
	void submit(const RenderState& state);
	// INEFFICIENT draw a quad immediately
	void submit(std::shared_ptr<VertexArray> va, 
			    hash_t shader_name, 
			    const ShaderParameters& params);
	// Request the renderer implementation to push a quad to current batch
	void draw_quad(const glm::vec2& position, 
				   const glm::vec2& scale,
				   const glm::vec3& color);
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
	virtual void push_quad(const glm::vec2& position, const glm::vec2& scale, const glm::vec3& color) = 0;
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
	SceneData scene_data_;

private:
	std::vector<uint16_t> batch_ttl_;
	bool profiling_enabled_ = false;
	QueryTimer* query_timer_;
};

class BatchRenderer2D: public Renderer2D
{
public:
	BatchRenderer2D(uint32_t max_batch_count=8192);
	~BatchRenderer2D() = default;

	virtual void set_batch_size(uint32_t value) override;

protected:
	virtual void flush() override;
	virtual void push_quad(const glm::vec2& position, const glm::vec2& scale, const glm::vec3& color) override;
	virtual void upload_batch() override;
	virtual void remove_unused_batches(uint32_t index) override;
	virtual uint32_t get_num_batches() override;
	virtual void create_batch() override;

private:
	std::vector<std::shared_ptr<VertexArray>> batches_;
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
	virtual void push_quad(const glm::vec2& position, const glm::vec2& scale, const glm::vec3& color) override;
	virtual void upload_batch() override;
	virtual void remove_unused_batches(uint32_t index) override;
	virtual uint32_t get_num_batches() override;
	virtual void create_batch() override;

private:
	struct InstanceData
	{
		glm::vec2 offset;
		glm::vec2 scale;
		glm::vec4 color; // Need a vec4 for alignment constraints
	};

	std::shared_ptr<VertexArray> quad_va_;
	std::vector<std::shared_ptr<ShaderStorageBuffer>> batches_;
	std::vector<InstanceData> instance_data_;
};

} // namespace erwin