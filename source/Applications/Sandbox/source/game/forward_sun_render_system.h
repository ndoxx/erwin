#pragma once

#include "erwin.h"
#include "editor/scene.h"
#include "game/game_components.h"

namespace erwin
{

class ForwardSunRenderSystem: public ComponentSystem<RequireAll<ComponentDirectionalLight, ComponentRenderableDirectionalLight>>
{
public:
	ForwardSunRenderSystem() = default;
	virtual ~ForwardSunRenderSystem() = default;

	virtual void update(const GameClock& clock) override final { }
	virtual void render() override final;
};


} // namespace erwin