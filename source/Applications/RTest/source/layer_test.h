#pragma once

#include "erwin.h"
#include "input/freefly_camera_system.h"

using namespace erwin;

class LayerTest: public Layer
{
public:
	friend class Sandbox;
	
	LayerTest();
	~LayerTest() = default;

	void setup_camera();
	virtual void on_imgui_render() override;

protected:
    virtual void on_attach() override;
    virtual void on_detach() override;
    virtual void on_update(erwin::GameClock& clock) override;
    virtual void on_render() override;
    virtual void on_commit() override;

    bool on_mouse_button_event(const erwin::MouseButtonEvent& event);
    bool on_mouse_moved_event(const erwin::MouseMovedEvent& event);
    bool on_window_resize_event(const erwin::WindowResizeEvent& event);
    bool on_window_moved_event(const erwin::WindowMovedEvent& event);
    bool on_mouse_scroll_event(const erwin::MouseScrollEvent& event);
    bool on_keyboard_event(const erwin::KeyboardEvent& event);

private:
	glm::vec4 bkg_color_;
    erwin::FreeflyCameraSystem camera_controller_;
};