#pragma once

#include "erwin.h"
#include "level/scene.h"
#include "editor/widget.h"
#include "editor/gizmo_system.h"
#include "editor/bounding_box_system.h"

namespace editor
{

class SceneViewWidget;
class SceneEditorLayer: public erwin::Layer
{
public:
	SceneEditorLayer();

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

	void add_widget(editor::Widget* widget);
	inline const auto& get_widgets() { return widgets_; }

protected:
	virtual void on_update(erwin::GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const erwin::MouseButtonEvent& event) override;
	virtual bool on_event(const erwin::WindowResizeEvent& event) override;
	virtual bool on_event(const erwin::MouseScrollEvent& event) override;
	virtual bool on_event(const erwin::KeyboardEvent& event) override;
	virtual bool on_event(const erwin::KeyTypedEvent& event) override;

private:
	std::vector<Widget*> widgets_;
	erwin::GizmoSystem gizmo_system_;
	erwin::BoundingBoxSystem bounding_box_system_;

	SceneViewWidget* scene_view_widget_ = nullptr;
};

} // namespace editor
