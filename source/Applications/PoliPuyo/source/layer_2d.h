#pragma once

#include "erwin.h"

using namespace erwin;

class Layer2D: public Layer
{
public:
	Layer2D();
	~Layer2D() = default;

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
	TextureAtlas connectivity_atlas_;
	TextureAtlas dungeon_atlas_;
	float tt_ = 0.f;
	bool enable_profiling_ = false;
};