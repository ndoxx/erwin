#include "entity/entity_manager.h"
#include "entity/component.h"
#include "entity/component_system.h"

namespace erwin
{

EntityManager::EntityManager()
{

}

EntityManager::~EntityManager()
{

}

void EntityManager::create_systems()
{

}

void EntityManager::update(const GameClock& clock)
{
	for(auto&& psys: systems_)
		psys->update(clock);
}

EntityID EntityManager::create_entity()
{
	return 0;
}

void EntityManager::destroy_entity(EntityID entity)
{

}


} // namespace erwin