#include "entity/entity_manager.h"
#include "entity/component.h"
#include "entity/component_system.h"
#include "debug/logger.h"
#include "core/eastl_new.h" // new overloads needed by EASTL (linker error otherwise)

namespace erwin
{

EntityID EntityManager::s_next_entity_id = 0;

EntityManager::EntityManager()
{

}

EntityManager::~EntityManager()
{
	// Delete all components
	// TMP: For now, entities own their components
	/*for(auto&& vec: components_)
		for(Component* pcmp: vec)
			delete pcmp;*/
	// Free all systems
	for(auto&& psys: systems_)
		delete psys;
}

void EntityManager::register_system(BaseComponentSystem* psys)
{
	systems_.push_back(psys);
}

void EntityManager::update(const GameClock& clock)
{
	for(auto&& psys: systems_)
		psys->update(clock);
}

Entity& EntityManager::create_entity()
{
	EntityID id = s_next_entity_id++;
	entities_.emplace(id,Entity(id));
	DLOG("entity",1) << "Created new entity:" << std::endl;
	DLOGI << "ID: " << id << std::endl;
	return entities_.at(id);
}

void EntityManager::submit_entity(EntityID entity)
{
	auto it = entities_.find(entity);
	if(it == entities_.end())
	{
		DLOGW("entity") << "Trying to submit non existing entity:" << std::endl;
		DLOGI << "ID: " << entity << std::endl;
		return;
	}
	// Register entity in all systems
	for(auto&& psys: systems_)
		psys->on_entity_submitted(it->second);
}

void EntityManager::destroy_entity(EntityID entity)
{
	auto it = entities_.find(entity);
	if(it == entities_.end())
	{
		DLOGW("entity") << "Trying to destroy non existing entity:" << std::endl;
		DLOGI << "ID: " << entity << std::endl;
		return;
	}
	// Remove entity from all systems
	for(auto&& psys: systems_)
		psys->on_entity_destroyed(it->second);
	entities_.erase(it);
}


} // namespace erwin