#pragma once

#include "erwin.h"

using namespace erwin;

class LayerTest: public Layer
{
public:
	friend class Sandbox;
	
	LayerTest();
	~LayerTest() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;

private:
	std::vector<std::future<erwin::PixelData>> futures_;
};