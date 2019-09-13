#pragma once

#include "render/buffer.h"
#include "render/shader.h"

#include "render/render_state.h" // For access to enums

namespace erwin
{

class Renderer2D
{
public:
	// Start batch
	static void begin_scene(uint32_t layer_index);
	// Stop batch
	static void end_scene();
	// Push a per-batch state change
	static void submit(const RenderState& state);
	// Push a per-instance draw command
	static void submit(std::shared_ptr<VertexArray> va, hash_t shader_name, const ShaderParameters& params);

	static ShaderBank shader_bank;

private:
	static uint32_t s_current_layer_;
};


} // namespace erwin