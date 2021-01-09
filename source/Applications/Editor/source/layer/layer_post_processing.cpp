#include "layer/layer_post_processing.h"
#include "level/scene_manager.h"
#include "render/renderer.h"
#include "render/renderer_pp.h"
#include "render/framebuffer_pool.h"

using namespace erwin;

namespace editor
{

PostProcessingLayer::PostProcessingLayer(erwin::Application& application):
Layer(application, "PostProcessing")
{

}

void PostProcessingLayer::on_imgui_render()
{

}

void PostProcessingLayer::on_attach()
{

}

void PostProcessingLayer::on_detach()
{

}

void PostProcessingLayer::on_update(erwin::GameClock&)
{

}

void PostProcessingLayer::on_render()
{
	if(scn::current_is_loaded())
	{
	    PostProcessingRenderer::bloom_pass("LBuffer"_h, 1);
	    PostProcessingRenderer::combine("LBuffer"_h, 0, true);
	    // PostProcessingRenderer::combine("SpriteBuffer"_h, 0, false);
	}
	else
	{
		// If no scene loaded, just clear host framebuffer
        FramebufferHandle fb = FramebufferPool::get_framebuffer("host"_h);
        Renderer::clear(0, fb, ClearFlags::CLEAR_COLOR_FLAG, {0.1f,0.1f,0.1f,1.f});
	}
}

void PostProcessingLayer::on_commit()
{

}


} // namespace editor