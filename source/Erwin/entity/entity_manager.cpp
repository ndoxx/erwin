#include "entity/entity_manager.h"
#include "entity/component.h"
#include "entity/component_system.h"
#include "debug/logger.h"
#include "core/eastl_new.h" // new overloads needed by EASTL (linker error otherwise)

namespace erwin
{

EntityID EntityManager::s_next_entity_id = 0;

EntityManager::~EntityManager()
{
	// Delete all component managers (and the components they hold)
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

void EntityManager::render()
{
	for(auto&& psys: systems_)
		psys->render();
}

EntityID EntityManager::create_entity()
{
	EntityID id = s_next_entity_id++;
	entities_.emplace(id,Entity(id));
	return id;
}

void EntityManager::submit_entity(EntityID entity_id)
{
	auto it = entities_.find(entity_id);
	if(it == entities_.end())
	{
		DLOGW("entity") << "Trying to submit non existing entity:" << std::endl;
		DLOGI << "ID: " << WCC('v') << entity_id << std::endl;
		return;
	}

	// Log stuff
#ifdef W_DEBUG
	DLOG("entity",0) << "Registered new entity:" << std::endl;
	DLOGI << "ID:        " << WCC('v') << entity_id << std::endl;
	for(auto&& [cid, pcmp]: it->second.get_components())
		DLOGI << "Component: " << WCC('n') << pcmp->get_debug_name() << std::endl;
#endif

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
		DLOGI << "ID: " << WCC('v') << entity_id << std::endl;
		return;
	}

	// Remove entity's components
	Entity& entity = it->second;
	for(auto&& [cid, pcmp]: entity.get_components())
		components_.at(pcmp->get_pool_index())->remove(entity_id);

	// Remove entity from all systems
	for(auto&& psys: systems_)
		psys->on_entity_destroyed(it->second);
	entities_.erase(it);

	DLOG("entity",0) << "Destroyed entity " << WCC('v') << entity_id << std::endl;
}


} // namespace erwin