#pragma once

#include <unordered_map>

#include "render/buffer.h"
#include "render/texture.h"
#include "render/shader.h"

#include "render/render_device.h" // TMP: just for access to enums

namespace erwin
{

class Renderer2D
{
public:
	enum class RenderTarget: uint8_t
	{
		DEFAULT = 0
	};

	struct ShaderParameters
	{
		void set_texture_slot(hash_t sampler_name, std::shared_ptr<Texture2D> texture);

		std::unordered_map<hash_t, std::shared_ptr<Texture2D>> texture_slots;
	};


	static void begin_scene();
	static void end_scene();

	static void set_render_target(RenderTarget target);
	static void set_cull_mode(CullMode cull_mode);

	static void submit(std::shared_ptr<VertexArray> va, hash_t shader_name, const ShaderParameters& params);

	static ShaderBank shader_bank;
};


} // namespace erwin