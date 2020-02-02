#pragma once

namespace erwin
{

class EntityManager;
class GameClock;
class Entity;
class BaseComponentSystem
{
public:
	explicit BaseComponentSystem() = default;
	virtual ~BaseComponentSystem() = default;

	virtual void update(const GameClock& clock) = 0;
	virtual void render() = 0;
	virtual void on_entity_submitted(const Entity& entity) = 0;
	virtual void on_entity_destroyed(const Entity& entity) = 0;
};


} // namespace erwin