#pragma once

#include "erwin.h"

using namespace erwin;

class LayerTest: public Layer
{
public:
	friend class Sandbox;
	
	LayerTest();
	~LayerTest() = default;

	virtual void on_imgui_render() override;

protected:
    virtual void on_attach() override;
    virtual void on_detach() override;
    virtual void on_update(erwin::GameClock& clock) override;
    virtual void on_render() override;
    virtual void on_commit() override;

	bool on_mouse_button_event(const MouseButtonEvent& event);
	bool on_window_resize_event(const WindowResizeEvent& event);
	bool on_mouse_scroll_event(const MouseScrollEvent& event);
};