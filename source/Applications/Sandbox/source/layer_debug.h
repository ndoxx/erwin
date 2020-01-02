#pragma once

#include "erwin.h"

using namespace erwin;

class DebugLayer: public Layer
{
public:
	friend class Sandbox;

	DebugLayer();
	~DebugLayer() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(GameClock& clock) override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;

private:
	bool texture_peek_ = false;
};