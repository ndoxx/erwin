#pragma once

#include "render/buffer.h"
#include "render/shader.h"
#include "render/render_state.h" // For access to enums
#include "render/query_timer.h" // TMP, will be moved to render thread

#include "glm/glm.hpp"

namespace erwin
{

struct RenderStats
{
	float render_time = 0.f;
	uint32_t batches = 0;
};

// Base class for 2D renderers
class Renderer2D
{
public:
	Renderer2D(uint32_t max_batch_count=8192);
	virtual ~Renderer2D();

	virtual void begin_scene(uint32_t layer_index) = 0;
	virtual void end_scene() = 0;

	virtual void submit(const RenderState& state) = 0;
	// TMP: DO NOT USE, for compat with one test layer in sandbox app
	virtual void submit(std::shared_ptr<VertexArray> va, 
					    hash_t shader_name, 
					    const ShaderParameters& params) = 0;

	virtual void draw_quad(const glm::vec2& position, 
				   	 	   const glm::vec2& scale,
				   	 	   const glm::vec3& color) = 0;

	virtual void set_batch_size(uint32_t value) = 0;


	inline void set_profiling_enabled(bool value = true) { profiling_enabled_ = value; }
	inline const RenderStats& get_stats() const          { return stats_; }

protected:
	inline void reset_stats() { stats_.render_time = 0.f; stats_.batches = 0; }

protected:
	uint32_t max_batch_count_;   // max number of quads in a batch
	uint32_t current_batch_;

	bool profiling_enabled_ = false;
	QueryTimer* query_timer_;
	RenderStats stats_;
};

class BatchRenderer2D: public Renderer2D
{
public:
	BatchRenderer2D(uint32_t max_batch_count=8192);
	~BatchRenderer2D();

	virtual void begin_scene(uint32_t layer_index) override;
	virtual void end_scene() override;

	virtual void submit(const RenderState& state) override;
	// TMP: DO NOT USE, for compat with one test layer in sandbox app
	virtual void submit(std::shared_ptr<VertexArray> va, hash_t shader_name, const ShaderParameters& params) override;

	virtual void draw_quad(const glm::vec2& position, 
				   		   const glm::vec2& scale,
				   		   const glm::vec3& color) override;

	virtual void set_batch_size(uint32_t value) override;

	static ShaderBank shader_bank;

private:
	// Add a new batch (vertex array) to the list
	void create_batch(const BufferLayout& layout);
	// Draw current batch and empty vertex / index list
	void flush();

private:
	std::vector<std::shared_ptr<VertexArray>> quad_batch_vas_;
	std::vector<float> vertex_list_;
};

class InstanceRenderer2D: public Renderer2D
{
public:
	InstanceRenderer2D(uint32_t max_batch_count=8192);
	~InstanceRenderer2D();

	virtual void begin_scene(uint32_t layer_index) override;
	virtual void end_scene() override;

	virtual void submit(const RenderState& state) override;
	// TMP: DO NOT USE, for compat with one test layer in sandbox app
	virtual void submit(std::shared_ptr<VertexArray> va, hash_t shader_name, const ShaderParameters& params) override { }

	virtual void draw_quad(const glm::vec2& position, 
				   		   const glm::vec2& scale,
				   		   const glm::vec3& color) override;

	virtual void set_batch_size(uint32_t value) override;

	static ShaderBank shader_bank;

private:
	// Add a new batch to the list
	void create_batch(uint32_t binding_point);
	// Draw current batch and empty vertex / index list
	void flush();

	struct InstanceData
	{
		glm::vec2 offset;
		glm::vec2 scale;
		glm::vec4 color; // Need a vec4 for alignment constraints
	};

private:
	std::shared_ptr<VertexArray> quad_va_;
	std::vector<std::shared_ptr<ShaderStorageBuffer>> batches_;
	std::vector<InstanceData> instance_data_;
};

} // namespace erwin