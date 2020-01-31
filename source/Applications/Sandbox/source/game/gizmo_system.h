#pragma once

#include "erwin.h"
#include "editor/scene.h"
#include "entity/component_transform.h"
#include "entity/component_bounding_box.h"

namespace erwin
{

class GizmoSystem: public ComponentSystem<RequireAll<ComponentOBB>>
{
public:
	GizmoSystem(EntityManager* manager);
	virtual ~GizmoSystem();
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final;
	virtual void render() override final;

private:
    erwin::ShaderHandle gizmo_shader_;
};


} // namespace erwin