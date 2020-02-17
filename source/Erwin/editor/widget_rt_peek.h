#pragma once

#include "editor/widget.h"
#include "render/handles.h"
#include "glm/glm.hpp"

namespace erwin
{
	class Scene;
}

namespace editor
{

class RTPeekWidget: public Widget
{
public:
	RTPeekWidget();
	virtual ~RTPeekWidget() = default;

	virtual void on_layer_render() override;

	uint32_t new_pane(const std::string& name);
	void register_texture(uint32_t pane_index, erwin::TextureHandle texture, const std::string& name, bool is_depth = false);
	void register_framebuffer(const std::string& framebuffer_name);

protected:
	virtual void on_imgui_render() override;
};

} // namespace editor