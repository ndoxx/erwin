#pragma once

#include "erwin.h"

using namespace erwin;

class DebugLayer: public Layer
{
public:
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
	bool enable_runtime_profiling_ = false;
	bool frame_profiling_ = false;
	int profile_num_frames_ = 60;
	int frames_counter_ = 0;
};