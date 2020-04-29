#pragma once

#include "erwin.h"
#include "layer/gui_layer.h"
#include "system/gizmo_system.h"
#include "system/bounding_box_system.h"

namespace editor
{

class SceneViewWidget;
class SceneEditorLayer: public GuiLayer
{
public:
	SceneEditorLayer();

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(erwin::GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const erwin::MouseButtonEvent& event) override;
	virtual bool on_event(const erwin::WindowResizeEvent& event) override;
	virtual bool on_event(const erwin::MouseScrollEvent& event) override;
	virtual bool on_event(const erwin::KeyboardEvent& event) override;
	virtual bool on_event(const erwin::KeyTypedEvent& event) override;

private:
	erwin::GizmoSystem gizmo_system_;
	erwin::BoundingBoxSystem bounding_box_system_;

	SceneViewWidget* scene_view_widget_ = nullptr;
};

} // namespace editor
