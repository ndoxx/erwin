#pragma once

#include "erwin.h"
#include "editor/scene.h"
#include "game/game_components.h"

namespace erwin
{

class ForwardSunRenderSystem: public ComponentSystem<RequireAll<ComponentDirectionalLight, ComponentRenderableDirectionalLight>>
{
public:
	ForwardSunRenderSystem(EntityManager* manager): BaseType(manager) {}
	virtual ~ForwardSunRenderSystem() = default;
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final;
	virtual void render() override final;

	inline void set_scene(erwin::Scene* p_scene) { p_scene_ = p_scene; }

private:
	erwin::Scene* p_scene_;
};


} // namespace erwin