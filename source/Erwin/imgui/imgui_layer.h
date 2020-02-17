#pragma once

#include "core/layer.h"

namespace erwin
{

class ImGuiLayer: public Layer
{
public:
	ImGuiLayer();
	~ImGuiLayer() = default;

	virtual void on_attach() override;
	virtual void on_detach() override;
	virtual void on_imgui_render() override;

	virtual bool on_event(const KeyboardEvent& event) override;
	virtual bool on_event(const KeyTypedEvent& event) override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;

	void begin();
	void end();

protected:
	virtual void on_update(GameClock& clock) override {}
};


} // namespace erwin