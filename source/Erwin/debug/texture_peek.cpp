#include "debug/texture_peek.h"
#include "debug/logger.h"
#include "core/core.h"
#include "render/renderer.h"
#include "render/common_geometry.h"
#include "imgui.h"

#include <vector>
#include <map>
#include <iostream>

namespace erwin
{

struct DebugTextureProperties
{
	TextureHandle texture;
	bool is_depth;
	std::string name;
};

struct DebugPane
{
	std::string name;
	std::vector<DebugTextureProperties> properties;
};

enum PeekFlags: uint32_t
{
	NONE = 0,
	TONE_MAP = 1,
	SPLIT_ALPHA = 2,
	INVERT = 4,
	DEPTH = 8,
};

struct PeekData
{
	uint32_t flags = 0;
	float split_pos = 0.5f;
	glm::vec2 texel_size;
	glm::vec4 channel_filter;
	glm::vec4 projection_parameters;
};

struct TexturePeekStorage
{
	std::vector<DebugPane> panes_;
	int current_pane_;
    int current_tex_;
    bool tone_map_;
    bool show_r_;
    bool show_g_;
    bool show_b_;
    bool invert_color_;
    bool split_alpha_;
	bool save_image_;
	bool enabled_;

	ShaderHandle peek_shader_;
	UniformBufferHandle pass_ubo_;
	uint64_t pass_state_;

	PeekData peek_data_;
};
static TexturePeekStorage s_storage;

void TexturePeek::init()
{
	// Create resources
    FramebufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    FramebufferPool::create_framebuffer("fb_texture_view"_h, make_scope<FbRatioConstraint>(), layout, false);

	// s_storage.peek_shader_ = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/texture_peek.glsl", "texture_peek");
	s_storage.peek_shader_ = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/texture_peek.spv", "texture_peek");
	s_storage.pass_ubo_ = Renderer::create_uniform_buffer("peek_layout", nullptr, sizeof(PeekData), DrawMode::Dynamic);
	Renderer::shader_attach_uniform_buffer(s_storage.peek_shader_, s_storage.pass_ubo_);

	// Initialize GUI
	s_storage.current_pane_ = 0;
	s_storage.current_tex_ = 0;
	s_storage.save_image_ = false;
	s_storage.show_r_ = true;
	s_storage.show_g_ = true;
	s_storage.show_b_ = true;
	s_storage.enabled_ = false;

	// State
	RenderState state;
	state.render_target = FramebufferPool::get_framebuffer("fb_texture_view"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = false;
	s_storage.pass_state_ = state.encode();
}

uint32_t TexturePeek::new_pane(const std::string& name)
{
	uint32_t pane_index = s_storage.panes_.size();
	s_storage.panes_.push_back(DebugPane{name});
	return pane_index;
}

void TexturePeek::register_texture(uint32_t pane_index, TextureHandle texture, const std::string& name, bool is_depth)
{
	W_ASSERT(pane_index < s_storage.panes_.size(), "Pane index out of bounds.");
	s_storage.panes_[pane_index].properties.push_back({texture, is_depth, name});
}

void TexturePeek::register_framebuffer(const std::string& framebuffer_name)
{
	hash_t hframebuffer = H_(framebuffer_name.c_str());
	FramebufferHandle fb = FramebufferPool::get_framebuffer(hframebuffer);
	bool has_depth = FramebufferPool::has_depth(hframebuffer);
	uint32_t ntex = Renderer::get_framebuffer_texture_count(fb);
	uint32_t pane_index = new_pane(framebuffer_name);

	if(has_depth)
		--ntex;

	for(uint32_t ii=0; ii<ntex; ++ii)
	{
		TextureHandle texture_handle = Renderer::get_framebuffer_texture(fb, ii);
		std::string tex_name = framebuffer_name + "_" + std::to_string(ii);
		register_texture(pane_index, texture_handle, tex_name, false);
	}

	if(has_depth)
	{
		TextureHandle texture_handle = Renderer::get_framebuffer_texture(fb, ntex);
		std::string tex_name = framebuffer_name + "_depth";
		register_texture(pane_index, texture_handle, tex_name, true);
	}
}

void TexturePeek::set_projection_parameters(const glm::vec4& proj_params)
{
	s_storage.peek_data_.projection_parameters = proj_params;
}

void TexturePeek::set_enabled(bool value)
{
	s_storage.enabled_ = value;
}

void TexturePeek::render()
{
#ifdef W_DEBUG
    if(!s_storage.enabled_ || s_storage.panes_.size() == 0)
    	return;

    DebugTextureProperties& props = s_storage.panes_[s_storage.current_pane_].properties[s_storage.current_tex_];
    TextureHandle current_texture = props.texture;
    
    // Update UBO data
    s_storage.peek_data_.texel_size = FramebufferPool::get_texel_size("fb_texture_view"_h);
    s_storage.peek_data_.flags = (s_storage.tone_map_ ? PeekFlags::TONE_MAP : PeekFlags::NONE)
    						   | (s_storage.invert_color_ ? PeekFlags::INVERT : PeekFlags::NONE)
    						   | (s_storage.split_alpha_ ? PeekFlags::SPLIT_ALPHA : PeekFlags::NONE)
    						   | (props.is_depth ? PeekFlags::DEPTH : PeekFlags::NONE);
    s_storage.peek_data_.channel_filter = { s_storage.show_r_, s_storage.show_g_, s_storage.show_b_, 1.f };

    // Submit draw call
	// WTF: If layer_id is set, command is dispatched at the end of frame like I would want it to,
	// but it fails miserably and the whole GUI disappears. I observed that setting the key this way
	// dispatches the command at the beginning of the frame, which works, despite showing the last
	// frame textures.
	static DrawCall dc(DrawCall::Indexed, 0, s_storage.pass_state_, s_storage.peek_shader_, CommonGeometry::get_vertex_array("quad"_h));
	dc.set_UBO(s_storage.pass_ubo_, &s_storage.peek_data_, sizeof(PeekData), DrawCall::CopyData);
	dc.set_texture(current_texture);
	dc.set_key_sequence(0);
	Renderer::submit(dc);
#endif
}

void TexturePeek::on_imgui_render(bool* p_open)
{
#ifdef W_DEBUG
    if(s_storage.panes_.size() == 0)
    	return;

    if(!ImGui::Begin("Texture peek", p_open))
    {
        ImGui::End();
        return;
    }

    // * Get render properties from GUI
    ImGui::BeginChild("##peekctl", ImVec2(0, 3*ImGui::GetItemsLineHeightWithSpacing()));
    ImGui::Columns(2, nullptr, false);

    if(ImGui::SliderInt("Panel", &s_storage.current_pane_, 0, s_storage.panes_.size()-1))
    {
        s_storage.current_tex_ = 0;
    }
    int ntex = s_storage.panes_[s_storage.current_pane_].properties.size();

    ImGui::SliderInt("Texture", &s_storage.current_tex_, 0, ntex-1);
    DebugTextureProperties& props = s_storage.panes_[s_storage.current_pane_].properties[s_storage.current_tex_];
    ImGui::Text("name: %s", props.name.c_str());
    ImGui::SameLine();
    if(ImGui::Button("Save to file"))
        s_storage.save_image_ = true;
    
    if(!props.is_depth)
    {
	    ImGui::NextColumn();
	    ImGui::Checkbox("Tone mapping", &s_storage.tone_map_);
	    ImGui::SameLine(); ImGui::Checkbox("R##0", &s_storage.show_r_);
	    ImGui::SameLine(); ImGui::Checkbox("G##0", &s_storage.show_g_);
	    ImGui::SameLine(); ImGui::Checkbox("B##0", &s_storage.show_b_);
	    ImGui::SameLine(); ImGui::Checkbox("Invert", &s_storage.invert_color_);

	    ImGui::Checkbox("Alpha split", &s_storage.split_alpha_);
	    if(s_storage.split_alpha_)
	    {
	        ImGui::SameLine();
	        ImGui::SliderFloat("Split pos.", &s_storage.peek_data_.split_pos, 0.f, 1.f);
	    }
	}

    ImGui::EndChild();

    // * Show image in window
    float winx = std::max(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - 8.f, 0.f);
    float winy = std::max(ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - 8.f, 0.f);

	// Retrieve the native framebuffer texture handle
	FramebufferHandle fb = FramebufferPool::get_framebuffer("fb_texture_view"_h);
	TextureHandle texture = Renderer::get_framebuffer_texture(fb, 0);
	void* framebuffer_texture_native = Renderer::get_native_texture_handle(texture);
    ImGui::GetWindowDrawList()->AddImage(framebuffer_texture_native,
                                         ImGui::GetCursorScreenPos(),
                                         ImVec2(winx, winy),
                                         ImVec2(0, 1), ImVec2(1, 0));

    // * Save image if needed
    if(s_storage.save_image_)
    {
        std::string filename = props.name + ".png";
        Renderer::framebuffer_screenshot(fb, filename);
        s_storage.save_image_ = false;
    }

    ImGui::End();
#endif
}

} // namespace erwin