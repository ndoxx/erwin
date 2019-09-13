#include "render/renderer_2d.h"

#include "core/core.h"
#include "render/render_device.h" // TMP: API access pushed to render thread at some point
#include "render/shader.h"
#include "platform/ogl_shader.h"

#include <iostream>

namespace erwin
{

ShaderBank Renderer2D::shader_bank;

void Renderer2D::ShaderParameters::set_texture_slot(hash_t sampler_name, std::shared_ptr<Texture2D> texture)
{
	texture_slots.insert(std::make_pair(sampler_name, texture));
}

void Renderer2D::begin_scene()
{

}

void Renderer2D::end_scene()
{

}

void Renderer2D::set_render_target(RenderTarget target)
{
	// TMP: direct rendering for now, will submit to render thread then
	if(target == RenderTarget::DEFAULT)
		Gfx::device->bind_default_frame_buffer();
	else
		W_ASSERT(false, "Custom render targets not supported yet!");
}

void Renderer2D::set_cull_mode(CullMode cull_mode)
{
	// TMP: direct rendering for now, will submit to render thread then
	Gfx::device->set_cull_mode(cull_mode);
}

void Renderer2D::submit(std::shared_ptr<VertexArray> va, hash_t shader_name, const ShaderParameters& params)
{
	// TMP: direct rendering for now, will submit to render thread then
	const Shader& shader = shader_bank.get(shader_name);
	shader.bind();

	// * Setup uniforms
	// Setup samplers
	for(auto&& [sampler, texture]: params.texture_slots)
	{
		uint32_t slot = shader.get_texture_slot(sampler);
    	texture->bind(slot);
    	static_cast<const OGLShader&>(shader).send_uniform<int>(sampler, slot);
	}

	// * Draw
    Gfx::device->draw_indexed(va);
}


} // namespace erwin