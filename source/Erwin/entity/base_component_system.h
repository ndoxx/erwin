#pragma once

namespace erwin
{

class EntityManager;
class GameClock;
class Entity;
class BaseComponentSystem
{
public:
	explicit BaseComponentSystem(EntityManager* manager);
	virtual ~BaseComponentSystem() = default;

	virtual void update(const GameClock& clock) = 0;
	virtual void on_entity_submitted(const Entity& entity) = 0;
	virtual void on_entity_destroyed(const Entity& entity) = 0;

protected:
	EntityManager* manager_;
};


} // namespace erwin