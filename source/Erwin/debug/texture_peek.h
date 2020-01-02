#pragma once

#include "render/handles.h"
#include "core/wtypes.h"
#include "glm/glm.hpp"

namespace erwin
{

/*
	TODO:
	[ ] Macroify access for fast stripping in release
*/

class TexturePeek
{
public:
	static void init();

	static uint32_t new_pane(const std::string& name);
	static void register_texture(uint32_t pane_index, TextureHandle texture, const std::string& name, bool is_depth = false);
	static void register_framebuffer(const std::string& framebuffer_name);
	static void set_projection_parameters(const glm::vec4& proj_params);

	static void render();
	static void on_imgui_render(bool* p_open);
};


} // namespace erwin