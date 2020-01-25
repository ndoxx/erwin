#pragma once

#include "erwin.h"
#include "game/scene.h"
#include "game/game_components.h"

namespace erwin
{

class PBRDeferredRenderSystem: public ComponentSystem<RequireAll<ComponentTransform3D, ComponentRenderablePBRDeferred>>
{
public:
	PBRDeferredRenderSystem(EntityManager* manager): BaseType(manager) {}
	virtual ~PBRDeferredRenderSystem() = default;
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final;
	virtual void render() override final;

	inline void set_scene(game::Scene* p_scene) { p_scene_ = p_scene; }

private:
	game::Scene* p_scene_;
};


} // namespace erwin