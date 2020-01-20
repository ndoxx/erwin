#pragma once

#include "erwin.h"
#include "scene.h"
#include "widget.h"

using namespace erwin;

class EditorLayer: public Layer
{
public:
	friend class Editor;
	
	EditorLayer(editor::Scene& scene);
	~EditorLayer() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

	inline void add_widget(hash_t name, editor::Widget* widget) { widgets_.insert(std::make_pair(name, widget)); }
	
	void show_dockspace_window(bool* p_open);

protected:
	virtual void on_update(GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;
	virtual bool on_event(const KeyboardEvent& event) override;

private:
	std::map<hash_t, editor::Widget*> widgets_;
	editor::Scene& scene_;
};