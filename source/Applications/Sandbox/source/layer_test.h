#pragma once

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"
#include "render/renderer_2d.h"

using namespace erwin;

class LayerTest: public Layer
{
public:
	LayerTest();
	~LayerTest() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(GameClock& clock) override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;

private:
	OrthographicCamera2DController camera_ctl_;
	TextureAtlas atlas_;
	std::vector<hash_t> tiles_;
	float tt_ = 0.f;
	bool trippy_mode_ = false;
	int len_grid_ = 50;
	bool enable_profiling_ = false;

	FramebufferHandle fb_handle_;
};