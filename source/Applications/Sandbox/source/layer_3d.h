#pragma once

#include "erwin.h"
#include <vector>

using namespace erwin;

struct PBRMaterialData
{
	glm::vec4 tint;
};

struct Cube
{
	ComponentTransform3D transform;
	Material material;
	PBRMaterialData material_data;
};

struct SunMaterialData
{
	glm::vec4 color;
};

class Layer3D: public Layer
{
public:
	friend class Sandbox;
	
	Layer3D();
	~Layer3D() = default;

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
	TextureGroupHandle tg_0_;
	TextureGroupHandle tg_1_;
	UniformBufferHandle pbr_material_ubo_;
	UniformBufferHandle sun_material_ubo_;
	ShaderHandle forward_opaque_pbr_;
	ShaderHandle forward_sun_;
	DirectionalLight dir_light_;

	Material sun_material_;
	SunMaterialData sun_material_data_;

	std::vector<Cube> scene_;
};