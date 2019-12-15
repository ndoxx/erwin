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
	entities_.clear(); // TMP

	// Delete all component managers
	for(auto&& pmgr: components_)
		delete pmgr;

	// Free all systems
	for(auto&& psys: systems_)
		delete psys;
}

void EntityManager::update(const GameClock& clock)
{
	for(auto&& psys: systems_)
		psys->update(clock);
}

EntityID EntityManager::create_entity()
{
	EntityID id = s_next_entity_id++;
	entities_.emplace(id,Entity(id));
	DLOG("entity",1) << "Created new entity:" << std::endl;
	DLOGI << "ID: " << id << std::endl;
	return id;
}

void EntityManager::submit_entity(EntityID entity_id)
{
	auto it = entities_.find(entity_id);
	if(it == entities_.end())
	{
		DLOGW("entity") << "Trying to submit non existing entity:" << std::endl;
		DLOGI << "ID: " << entity_id << std::endl;
		return;
	}
	// Register entity in all systems
	for(auto&& psys: systems_)
		psys->on_entity_submitted(it->second);
}

void EntityManager::destroy_entity(EntityID entity_id)
{
	auto it = entities_.find(entity_id);
	if(it == entities_.end())
	{
		DLOGW("entity") << "Trying to destroy non existing entity:" << std::endl;
		DLOGI << "ID: " << entity_id << std::endl;
		return;
	}

	// Remove entity's components
	Entity& entity = it->second;
	for(auto&& [cid, pcmp]: entity.get_components())
	{
		size_t mgr_index = components_lookup_.at(cid);
		components_.at(mgr_index)->remove(entity_id);
	}

	// Remove entity from all systems
	for(auto&& psys: systems_)
		psys->on_entity_destroyed(it->second);
	entities_.erase(it);
}


} // namespace erwin