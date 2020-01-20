#pragma once

#include "erwin.h"
#include "scene.h"

using namespace erwin;

struct PBRMaterialData
{
	inline void enable_emissivity() { flags |= (1<<0); }

	glm::vec4 tint;
	int flags;
	float emissive_scale;
};

struct Cube
{
	ComponentTransform3D transform;
	Material material;
	PBRMaterialData material_data;
};

class LayerTest: public Layer
{
public:
	friend class Sandbox;
	
	LayerTest(editor::Scene& scene);
	~LayerTest() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const WindowMovedEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;
	virtual bool on_event(const MouseMovedEvent& event) override;
	virtual bool on_event(const KeyboardEvent& event) override;

private:
	TextureGroupHandle tg_;
	UniformBufferHandle pbr_material_ubo_;
	ShaderHandle deferred_pbr_;
	ShaderHandle background_shader_;
	DirectionalLight dir_light_;

	Cube emissive_cube_;
	PostProcessingData pp_data_;

	editor::Scene& scene_;
};