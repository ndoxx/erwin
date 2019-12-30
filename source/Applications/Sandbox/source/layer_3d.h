#pragma once

#include "erwin.h"

using namespace erwin;

class Layer3D: public Layer
{
public:
	Layer3D();
	~Layer3D() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(GameClock& clock) override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;
	virtual bool on_event(const MouseMovedEvent& event) override;

private:
	PerspectiveFreeflyController camera_ctl_;
	MaterialHandle material_;
	ShaderHandle forward_opaque_pbr_;
	float tt_ = 0.f;
};