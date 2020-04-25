#include "editor/layer_editor_background.h"
#include "asset/asset_manager.h"

using namespace erwin;

namespace editor
{

EditorBackgroundLayer::EditorBackgroundLayer():
Layer("EditorBackgroundLayer")
{

}

void EditorBackgroundLayer::on_attach()
{
	background_shader_ = AssetManager::load_shader("shaders/background.glsl");
}

void EditorBackgroundLayer::on_detach()
{
	Renderer::destroy(background_shader_);
}

void EditorBackgroundLayer::on_render()
{
	// Background is the last layer, perform post processing here
    PostProcessingRenderer::bloom_pass("LBuffer"_h, 1);
    PostProcessingRenderer::combine("LBuffer"_h, 0, true);
	// PostProcessingRenderer::combine("SpriteBuffer"_h, 0, false);

	// WTF: we must draw something to the default framebuffer or else, whole screen is blank
	RenderState state;
	state.render_target = Renderer::default_render_target();
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;

	VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);
	SortKey key;
	key.set_sequence(0, Renderer::next_layer_id(), background_shader_);
	DrawCall dc(DrawCall::Indexed, state.encode(), background_shader_, quad);
	Renderer::submit(key.encode(), dc);
}

} // namespace editor