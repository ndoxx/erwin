#pragma once

#include "erwin.h"
#include "editor/scene.h"
#include "game/game_components.h"

namespace erwin
{

class PBRDeferredRenderSystem: public ComponentSystem<RequireAll<ComponentTransform3D, ComponentRenderablePBR>>
{
public:
	PBRDeferredRenderSystem(EntityManager* manager): BaseType(manager) {}
	virtual ~PBRDeferredRenderSystem() = default;
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final;
	virtual void render() override final;

	inline void set_scene(erwin::Scene* p_scene) { p_scene_ = p_scene; }

private:
	erwin::Scene* p_scene_;
};


} // namespace erwin