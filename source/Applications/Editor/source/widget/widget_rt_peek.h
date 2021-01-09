#pragma once

#include "glm/glm.hpp"
#include "render/handles.h"
#include "widget/widget.h"

namespace erwin
{
class Scene;
}

namespace editor
{

class RTPeekWidget : public Widget
{
public:
    RTPeekWidget(erwin::EventBus&);
    virtual ~RTPeekWidget() = default;

    virtual void on_update(const erwin::GameClock&) override;
    virtual void on_layer_render() override;

    size_t new_pane(const std::string& name);
    void register_texture(size_t pane_index, erwin::TextureHandle texture, const std::string& name,
                          bool is_depth = false);
    void register_framebuffer(const std::string& framebuffer_name);

protected:
    virtual void on_imgui_render() override;
};

} // namespace editor