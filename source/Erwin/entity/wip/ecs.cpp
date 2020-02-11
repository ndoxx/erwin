#include "entity/wip/ecs.h"
#include "memory/arena.h"

namespace erwin
{
ROBUST_HANDLE_DEFINITION(EntityHandle, wip::k_max_entities);
}

using namespace erwin;

namespace wip
{

using Components    = eastl::hash_map<ComponentID, ComponentStorage, eastl::hash<ComponentID>, eastl::equal_to<ComponentID>/*, PooledEastlAllocator*/>;
using ComponentsMap = eastl::hash_map<ComponentID, Component*>;
using Registry      = eastl::hash_map<EntityIdx, ComponentsMap>;

static struct
{
	LinearArena handle_arena;
	Components component_storage;
	Registry registry;
} s_storage;

ComponentStorage::ComponentStorage(erwin::memory::HeapArea& area, size_t max_components, size_t component_size, const std::string& debug_name):
pool_(area, component_size + erwin::PoolArena::DECORATION_SIZE, max_components, debug_name.c_str())
{

}

ComponentStorage::~ComponentStorage()
{
	for(auto&& pcmp: components_)
		W_DELETE(pcmp, pool_);

	pool_.shutdown();
}

void ComponentStorage::remove(erwin::EntityHandle entity)
{
	auto it = lookup_.find(entity.index);
	W_ASSERT_FMT(it != lookup_.end(), "Cannot find entity %lu in manager", entity.index);

	// Use swap trick for fast removal
	size_t index = it->second;
	erwin::W_DELETE(components_[index], pool_);
	components_[index] = std::move(components_.back());
	components_.pop_back();
	size_t back_entity_id = components_[index]->get_parent_entity().index;
	// Update lookup table at index
	lookup_[back_entity_id] = index;
}


void ECS::init(memory::HeapArea& area)
{
	s_storage.handle_arena.init(area, 512_kB, "EntityHandleArena");
	EntityHandle::init_pool(s_storage.handle_arena);
}

void ECS::shutdown()
{
	s_storage.component_storage.clear();
	EntityHandle::destroy_pool(s_storage.handle_arena);
}

EntityHandle ECS::create_entity()
{
	EntityHandle handle = EntityHandle::acquire();
	W_ASSERT(handle.is_valid(), "EntityHandle pool is full.");
	s_storage.registry.emplace(handle.index, ComponentsMap());
	return handle;
}

void ECS::destroy_entity(EntityHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "Invalid EntityHandle: (%lu,%lu)", handle.index, handle.counter);

	// Destroy all components held by entity
	auto it = s_storage.registry.find(handle.index);
	for(auto&& [cid, pcmp]: it->second)
		s_storage.component_storage.at(cid).remove(handle);

	s_storage.registry.erase(it);
	handle.release();
}


void ECS::create_component_storage_impl(memory::HeapArea& area, size_t max_components, size_t component_size, ComponentID cid, const std::string& debug_name)
{
	s_storage.component_storage.emplace(cid, ComponentStorage(area, max_components, component_size, debug_name));
}

ComponentStorage& ECS::get_storage(ComponentID cid)
{
	return s_storage.component_storage.at(cid);
}

void ECS::add_entity_component(erwin::EntityHandle entity, ComponentID cid, Component* pcmp)
{
	auto it = s_storage.registry.find(entity.index);
	it->second.emplace(cid, pcmp);
}

Component* ECS::get_component(erwin::EntityHandle entity, ComponentID cid)
{
	return s_storage.registry.at(entity.index).at(cid);
}



} // namespace wip