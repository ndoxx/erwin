#pragma once

/*
	Original design by Rez Bot: https://www.youtube.com/watch?v=5KugyHKsXLQ
*/

#include <vector>
#include <map>

#include "EASTL/hash_map.h"
#include "EASTL/vector.h"
#include "entity/entity.h"

namespace erwin
{

class Component;
class BaseComponentSystem;
class GameClock;
class EntityManager
{
public:
	EntityManager(const EntityManager&) = delete;
	EntityManager(EntityManager&&) = delete;
	EntityManager& operator=(const EntityManager&) = delete;
	EntityManager& operator=(EntityManager&&) = delete;
	EntityManager();
	~EntityManager();

	void create_systems();
	void update(const GameClock& clock);
	EntityID create_entity();
	void destroy_entity(EntityID entity);

private:
	using Entities   = eastl::hash_map<EntityID, Entity>;
	using Components = eastl::vector<eastl::vector<Component*>>;
	using Systems    = eastl::vector<BaseComponentSystem*>;

	Entities entities_;
	Components components_;
	Systems systems_;
};


} // namespace erwin