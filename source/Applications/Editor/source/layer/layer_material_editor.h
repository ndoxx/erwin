#pragma once

#include "erwin.h"
#include "layer/gui_layer.h"

namespace editor
{

class MaterialViewWidget;
class MaterialEditorLayer: public GuiLayer
{
public:
	MaterialEditorLayer();

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(erwin::GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const erwin::MouseButtonEvent& event) override;
	virtual bool on_event(const erwin::MouseMovedEvent& event) override;
	virtual bool on_event(const erwin::WindowResizeEvent& event) override;
	virtual bool on_event(const erwin::MouseScrollEvent& event) override;
	virtual bool on_event(const erwin::KeyboardEvent& event) override;
	virtual bool on_event(const erwin::KeyTypedEvent& event) override;

private:
	MaterialViewWidget* material_view_widget_ = nullptr;
};

} // namespace editor
