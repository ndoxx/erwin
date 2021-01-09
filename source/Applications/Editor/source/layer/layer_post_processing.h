#pragma once

#include "core/layer.h"

namespace editor
{

class PostProcessingLayer : public erwin::Layer
{
public:
    PostProcessingLayer(erwin::Application&);
    
    virtual void on_imgui_render() override;

protected:
    virtual void on_attach() override;
    virtual void on_detach() override;
    virtual void on_update(erwin::GameClock& clock) override;
    virtual void on_render() override;
    virtual void on_commit() override;
};

} // namespace editor
