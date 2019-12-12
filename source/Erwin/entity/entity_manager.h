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
	NON_COPYABLE(EntityManager);
	NON_MOVABLE(EntityManager);
	EntityManager();
	~EntityManager();

	void register_system(BaseComponentSystem* psys);
	void update(const GameClock& clock);

	Entity& create_entity();
	void submit_entity(EntityID entity);
	void destroy_entity(EntityID entity);

private:
	using Entities   = eastl::hash_map<EntityID, Entity>;
	using Components = eastl::vector<eastl::vector<Component*>>;
	using Systems    = eastl::vector<BaseComponentSystem*>;

	Entities entities_;
	// Components components_; // TMP: For now, entities own their components
	Systems systems_;

	static EntityID s_next_entity_id;
};


} // namespace erwin