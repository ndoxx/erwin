#pragma once

#include "widget.h"
#include "erwin.h"
#include "glm/glm.hpp"

namespace game
{
	class Scene;
}

namespace editor
{

class RTPeekWidget: public Widget
{
public:
	RTPeekWidget(game::Scene& scene);
	virtual ~RTPeekWidget();

	virtual void on_layer_render() override;

	uint32_t new_pane(const std::string& name);
	void register_texture(uint32_t pane_index, erwin::TextureHandle texture, const std::string& name, bool is_depth = false);
	void register_framebuffer(const std::string& framebuffer_name);

protected:
	virtual void on_imgui_render() override;

private:
	game::Scene& scene_;
};

} // namespace editor