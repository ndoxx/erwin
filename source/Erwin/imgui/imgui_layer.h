#pragma once

#include "core/layer.h"

namespace erwin
{

/*
	TODO:
	Not a true Layer anymore as it's not pushed in the layer stack.
	EditorLayer stole its event handling code, this only serves to
	render ImGui stuff, maybe a rename and a small refactor are in order.
*/
class ImGuiLayer: public Layer
{
public:
	ImGuiLayer(Application&);
	~ImGuiLayer() = default;

	virtual void on_attach() override;
	virtual void on_detach() override;
	virtual void on_imgui_render() override;

	void begin();
	void end();

protected:
	virtual void on_update(GameClock&) override {}
};


} // namespace erwin