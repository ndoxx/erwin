#pragma once

#include "render/buffer.h"
#include "render/shader.h"
#include "render/render_state.h" // For access to enums
#include "render/query_timer.h" // TMP, will be moved to render thread
#include "math/math3d.h"

namespace erwin
{

class Renderer2D
{
public:
	Renderer2D();
	~Renderer2D();

	void begin_scene(uint32_t layer_index);
	void end_scene();

	// Push a per-batch state change
	void submit(const RenderState& state);
	// Push a per-instance draw command
	void submit(std::shared_ptr<VertexArray> va, hash_t shader_name, const ShaderParameters& params);

	inline void set_profiling_enabled(bool value = true) { profiling_enabled_ = value; }

	static ShaderBank shader_bank;

private:
	uint32_t current_layer_ = 0;
	bool profiling_enabled_ = false;

	QueryTimer* query_timer_;
};


class BatchRenderer2D
{
public:
	BatchRenderer2D(uint32_t num_batches, uint32_t max_batch_count);
	~BatchRenderer2D();

	void begin_scene(uint32_t layer_index);
	void end_scene();

	void submit(const RenderState& state);

	void draw_quad(const math::vec2& position, 
				   const math::vec2& scale,
				   const math::vec3& color);

	static ShaderBank shader_bank;

private:
	std::vector<std::shared_ptr<VertexArray>> quad_batch_vas_;
	uint32_t max_batch_count_; // max number of quads in a batch
	uint32_t max_batches_;     // max number of batches
	uint32_t current_batch_ = 0;
	uint32_t current_batch_count_ = 0;
	uint32_t current_batch_v_offset_ = 0;
	uint32_t current_batch_i_offset_ = 0;
};

} // namespace erwin