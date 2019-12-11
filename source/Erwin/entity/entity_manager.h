#pragma once

#include <vector>
#include <map>

#include "EASTL/hash_map.h"
#include "EASTL/vector.h"
#include "entity/entity.h"
#include "entity/component.h"
#include "entity/component_system.h"

namespace erwin
{

class EntityManager
{
public:
	void create_systems();
	void update(const GameClock& clock);
	EntityID create_entity();
	void destroy_entity(EntityID entity);

private:
	using Entities   = eastl::hash_map<EntityID, Entity>;
	using Components = eastl::vector<eastl::vector<Component*>>;
	using Systems    = eastl::vector<ComponentSystem*>;

	Entities entities_;
	Components components_;
	Systems systems_;
};


} // namespace erwin