#pragma once

#include "erwin.h"
#include "editor/scene.h"
#include "entity/component_transform.h"
#include "entity/component_bounding_box.h"
#include "event/window_events.h"

namespace erwin
{

class BoundingBoxSystem: public ComponentSystem<RequireAll<ComponentTransform3D, ComponentOBB>>
{
public:
	BoundingBoxSystem(EntityManager* manager);
	virtual ~BoundingBoxSystem() = default;
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final;
	virtual void render() override final;

	bool on_ray_scene_query_event(const RaySceneQueryEvent& event);
};


} // namespace erwin