#pragma once

#include "erwin.h"
#include "editor/scene.h"
#include "editor/widget.h"

namespace editor
{

class EditorLayer: public erwin::Layer
{
public:
	friend class Editor;
	
	struct MenuDescriptor
	{
		std::string name;
		std::vector<Widget*> widgets;
	};

	EditorLayer();

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

	uint32_t add_menu(const std::string& menu_name);
	void add_widget(uint32_t menu, editor::Widget* widget);
	
	void show_dockspace_window(bool* p_open);

protected:
	virtual void on_update(erwin::GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const erwin::MouseButtonEvent& event) override;
	virtual bool on_event(const erwin::WindowResizeEvent& event) override;
	virtual bool on_event(const erwin::MouseScrollEvent& event) override;
	virtual bool on_event(const erwin::KeyboardEvent& event) override;

private:
	std::vector<MenuDescriptor> menus_;
	erwin::ShaderHandle background_shader_;
};

} // namespace editor
