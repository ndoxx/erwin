#pragma once

#include "erwin.h"
#include "game/scene.h"
#include "game/game_components.h"

namespace erwin
{

class ForwardSunRenderSystem: public ComponentSystem<ComponentDirectionalLight, ComponentRenderableDirectionalLight>
{
	using BaseType = ComponentSystem<ComponentDirectionalLight, ComponentRenderableDirectionalLight>;

public:
	ForwardSunRenderSystem(EntityManager* manager): BaseType(manager) {}
	virtual ~ForwardSunRenderSystem() = default;
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final;
	virtual void render() override final;

	inline void set_scene(game::Scene* p_scene) { p_scene_ = p_scene; }

private:
	game::Scene* p_scene_;
};


} // namespace erwin