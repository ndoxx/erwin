#pragma once

#include "render/handles.h"
#include "core/wtypes.h"

namespace erwin
{

/*
	TODO:
	[X] Create a framebuffer the render() method will draw to
	[X] In main renderer, create a debug method to access native texture renderer handles
		[X] Save framebuffer's texture renderer handle to assign it to ImGui frame
	[ ] Copy from WCore:
		[X] The peek shader
		[X] The "Framebuffer Peek" GUI
		[ ] Depth texture's special handling code
		[ ] The framebuffer capture code
	[ ] Macroify access for fast stripping in release
	[X] Sandbox debug layer is responsible for the texture view pass
*/

class TexturePeek
{
public:
	static void init();

	static uint32_t new_pane(const std::string& name);
	static void register_texture(uint32_t pane_index, TextureHandle texture, const std::string& name, bool is_depth = false);
	static void register_framebuffer(const std::string& framebuffer_name);

	static void render();
	static void on_imgui_render();
};


} // namespace erwin