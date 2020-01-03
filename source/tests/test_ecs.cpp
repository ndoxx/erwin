#include "catch2/catch.hpp"
#include "glm/glm.hpp"
#include "memory/memory.hpp"
#include "memory/pool_allocator.h"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"
#include "entity/component.h"
#include "entity/component_system.h"
#include "entity/entity_manager.h"
#include "core/game_clock.h"

using namespace erwin;

class AComponent: public Component
{
public:
	COMPONENT_DECLARATION(AComponent);

	int a;

	AComponent(): a(1) {}
	virtual bool init(void* description) override final { return true; }
};
COMPONENT_DEFINITION(AComponent);

class BComponent: public Component
{
public:
	COMPONENT_DECLARATION(BComponent);

	int b1;
	int b2;

	BComponent(): b1(10), b2(10) {}
	virtual bool init(void* description) override final { return true; }
};
COMPONENT_DEFINITION(BComponent);

class CComponent: public Component
{
public:
	COMPONENT_DECLARATION(CComponent);

	long c;

	CComponent(): c(0) {}
	virtual bool init(void* description) override final { return true; }
};
COMPONENT_DEFINITION(CComponent);

// Systems
class ABAdderSystem: public ComponentSystem<AComponent, BComponent>
{
	using BaseType = ComponentSystem<AComponent, BComponent>;

public:
	ABAdderSystem(EntityManager* manager): BaseType(manager) {}
	virtual ~ABAdderSystem() = default;
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final
	{
		for(auto&& cmp_tuple: components_)
		{
			AComponent* acmp = eastl::get<AComponent*>(cmp_tuple);
			BComponent* bcmp = eastl::get<BComponent*>(cmp_tuple);

			// Do stuff
			EntityID eid = acmp->get_parent_entity();
			DLOG("entity",1) << "ABAdderSystem: (" << eid << ") [" << bcmp->b1 << "," << bcmp->b2 << "]";
			bcmp->b1 += acmp->a; 
			bcmp->b2 -= acmp->a; 
			DLOG("entity",1) << " -> [" << bcmp->b1 << "," << bcmp->b2 << "]" << std::endl;
		}
	}
};

class BCAdderSystem: public ComponentSystem<BComponent, CComponent>
{
	using BaseType = ComponentSystem<BComponent, CComponent>;

public:
	BCAdderSystem(EntityManager* manager): BaseType(manager) {}
	virtual ~BCAdderSystem() = default;
	virtual bool init() override final { return true; }

	virtual void update(const GameClock& clock) override final
	{
		for(auto&& cmp_tuple: components_)
		{
			BComponent* bcmp = eastl::get<BComponent*>(cmp_tuple);
			CComponent* ccmp = eastl::get<CComponent*>(cmp_tuple);

			// Do stuff
			EntityID eid = bcmp->get_parent_entity();
			DLOG("entity",1) << "BCAdderSystem: (" << eid << ") " << ccmp->c;
			ccmp->c += (bcmp->b1 + bcmp->b2) / 2; 
			DLOG("entity",1) << " -> " << ccmp->c << std::endl;
		}
	}
};


class ECSFixture
{
public:

	ECSFixture():
	area(1_MB)
	{
		size_t N = 8;
		mgr.create_component_manager<AComponent>(area, N);
		mgr.create_component_manager<BComponent>(area, N);
		mgr.create_component_manager<CComponent>(area, N);
		mgr.create_system<ABAdderSystem>();
		mgr.create_system<BCAdderSystem>();

		// Entities that contain A and B components
		for(int ii=0; ii<4; ++ii)
		{
			EntityID ent = mgr.create_entity();
			mgr.create_component<AComponent>(ent);
			mgr.create_component<BComponent>(ent);
			mgr.submit_entity(ent);
		}
		// Entities that contain B and C components
		for(int ii=0; ii<4; ++ii)
		{
			EntityID ent = mgr.create_entity();
			mgr.create_component<BComponent>(ent);
			mgr.create_component<CComponent>(ent);
			mgr.submit_entity(ent);
		}
	}

protected:
	memory::HeapArea area;
	EntityManager mgr;
};

TEST_CASE_METHOD(ECSFixture, "Component filtering test: check processed entities", "[ecs]")
{
	GameClock game_clock;
	mgr.update(game_clock);
	area.debug_hex_dump(std::cout);

	for(int ii=0; ii<4; ++ii)
	{
		Entity& ent = mgr.get_entity(ii);
		REQUIRE(ent.get_component<BComponent>()->b1 == 11);
		REQUIRE(ent.get_component<BComponent>()->b2 == 9);
	}
	for(int ii=4; ii<8; ++ii)
	{
		Entity& ent = mgr.get_entity(ii);
		REQUIRE(ent.get_component<CComponent>()->c == 10);
	}
}

TEST_CASE_METHOD(ECSFixture, "Component filtering test: check ignored entities", "[ecs]")
{
	std::vector<EntityID> ids;

	// Entities that contain A and C components (should be ignored)
	for(int ii=0; ii<4; ++ii)
	{
		EntityID ent = mgr.create_entity();
		mgr.create_component<AComponent>(ent);
		mgr.create_component<CComponent>(ent);
		mgr.submit_entity(ent);
		ids.push_back(ent);
	}

	GameClock game_clock;
	mgr.update(game_clock);
	area.debug_hex_dump(std::cout);

	for(EntityID id: ids)
	{
		Entity& ent = mgr.get_entity(id);
		REQUIRE(ent.get_component<CComponent>()->c == 0);
	}
}