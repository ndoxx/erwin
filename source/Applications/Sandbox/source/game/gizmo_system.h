#pragma once

#include "erwin.h"
#include "editor/scene.h"
#include "entity/component_transform.h"
#include "entity/component_bounding_box.h"
#include "event/window_events.h"

namespace erwin
{

class GizmoSystem: public ComponentSystem<RequireAll<ComponentOBB>>
{
public:
	GizmoSystem();
	virtual ~GizmoSystem();
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final;
	virtual void render() override final;

	bool on_ray_scene_query_event(const RaySceneQueryEvent& event);

private:
    erwin::ShaderHandle gizmo_shader_;
    erwin::UniformBufferHandle gizmo_ubo_;
    erwin::Material gizmo_material_;
    int selected_part_;

    struct GizmoData
    {
    	int selected;
    } gizmo_data_;
};


} // namespace erwin