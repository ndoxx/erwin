#pragma once

#include "erwin.h"
#include "editor/scene.h"
#include "entity/component_transform.h"
#include "entity/component_bounding_box.h"

namespace erwin
{

class DebugRenderSystem: public ComponentSystem<RequireAll<ComponentOBB>>
{
public:
	DebugRenderSystem(EntityManager* manager): BaseType(manager) {}
	virtual ~DebugRenderSystem() = default;
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final;
	virtual void render() override final;

	inline void set_scene(erwin::Scene* p_scene) { p_scene_ = p_scene; }

private:
	erwin::Scene* p_scene_;
};


} // namespace erwin