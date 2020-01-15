#pragma once

#include "erwin.h"
#include "common.h"

using namespace erwin;

class Layer3DDeferred: public Layer
{
public:
	friend class Sandbox;
	
	Layer3DDeferred();
	~Layer3DDeferred() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;
	virtual bool on_event(const MouseMovedEvent& event) override;

private:
	PerspectiveFreeflyController camera_ctl_;
	TextureGroupHandle tg_;
	UniformBufferHandle pbr_material_ubo_;
	ShaderHandle deferred_pbr_;
	DirectionalLight dir_light_;
	Cube emissive_cube_;
};