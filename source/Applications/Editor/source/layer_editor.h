#pragma once

#include "erwin.h"
#include "game/scene.h"
#include "widget.h"

class EditorLayer: public erwin::Layer
{
public:
	friend class Editor;
	
	EditorLayer(game::Scene& scene);
	~EditorLayer() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

	void add_widget(editor::Widget* widget);
	
	void show_dockspace_window(bool* p_open);

protected:
	virtual void on_update(erwin::GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const erwin::MouseButtonEvent& event) override;
	virtual bool on_event(const erwin::WindowResizeEvent& event) override;
	virtual bool on_event(const erwin::MouseScrollEvent& event) override;
	virtual bool on_event(const erwin::KeyboardEvent& event) override;

private:
	std::map<erwin::hash_t, editor::Widget*> widgets_;
	erwin::ShaderHandle background_shader_;
	game::Scene& scene_;
};