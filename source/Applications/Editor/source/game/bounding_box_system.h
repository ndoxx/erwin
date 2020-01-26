#pragma once

#include "erwin.h"
#include "editor/scene.h"
#include "entity/component_transform.h"
#include "entity/component_bounding_box.h"

namespace erwin
{

class BoundingBoxSystem: public ComponentSystem<RequireAll<ComponentTransform3D, ComponentOBB>>
{
public:
	BoundingBoxSystem(EntityManager* manager): BaseType(manager) {}
	virtual ~BoundingBoxSystem() = default;
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final;
};


} // namespace erwin