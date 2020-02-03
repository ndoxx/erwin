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
class ABAdderSystem: public ComponentSystem<RequireAll<AComponent, BComponent>>
{
public:
	ABAdderSystem() = default;
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

class BCAdderSystem: public ComponentSystem<RequireAll<BComponent, CComponent>>
{
public:
	BCAdderSystem() = default;
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
		size_t N = 16;
		ECS::create_component_manager<AComponent>(area, N);
		ECS::create_component_manager<BComponent>(area, N);
		ECS::create_component_manager<CComponent>(area, N);
		ECS::create_system<ABAdderSystem>();
		ECS::create_system<BCAdderSystem>();

		// Entities that contain A and B components
		for(int ii=0; ii<4; ++ii)
		{
			EntityID ent = ECS::create_entity();
			ECS::create_component<AComponent>(ent);
			ECS::create_component<BComponent>(ent);
			ECS::submit_entity(ent);
		}
		// Entities that contain B and C components
		for(int ii=0; ii<4; ++ii)
		{
			EntityID ent = ECS::create_entity();
			ECS::create_component<BComponent>(ent);
			ECS::create_component<CComponent>(ent);
			ECS::submit_entity(ent);
		}
	}

	~ECSFixture()
	{
		ECS::shutdown();
	}


protected:
	memory::HeapArea area;
};

TEST_CASE_METHOD(ECSFixture, "Component filtering test: check processed entities", "[ecs]")
{
	GameClock game_clock;
	ECS::update(game_clock);
	area.debug_hex_dump(std::cout);

	for(int ii=0; ii<4; ++ii)
	{
		Entity& ent = ECS::get_entity(ii);
		REQUIRE(ent.get_component<BComponent>()->b1 == 11);
		REQUIRE(ent.get_component<BComponent>()->b2 == 9);
	}
	for(int ii=4; ii<8; ++ii)
	{
		Entity& ent = ECS::get_entity(ii);
		REQUIRE(ent.get_component<CComponent>()->c == 10);
	}
}

TEST_CASE_METHOD(ECSFixture, "Component filtering test: check ignored entities", "[ecs]")
{
	std::vector<EntityID> ids;

	// Entities that contain A and C components (should be ignored)
	for(int ii=0; ii<4; ++ii)
	{
		EntityID ent = ECS::create_entity();
		ECS::create_component<AComponent>(ent);
		ECS::create_component<CComponent>(ent);
		ECS::submit_entity(ent);
		ids.push_back(ent);
	}

	GameClock game_clock;
	ECS::update(game_clock);
	area.debug_hex_dump(std::cout);

	for(EntityID id: ids)
	{
		Entity& ent = ECS::get_entity(id);
		REQUIRE(ent.get_component<CComponent>()->c == 0);
	}
}

TEST_CASE_METHOD(ECSFixture, "Adding a component dynamically", "[ecs]")
{
	EntityID ent = ECS::create_entity();
	auto& a = ECS::create_component<AComponent>(ent);
	auto& b = ECS::create_component<BComponent>(ent);
	ECS::submit_entity(ent);

	GameClock game_clock;
	ECS::update(game_clock);
	REQUIRE(b.b1 == 11);
	REQUIRE(b.b2 == 9);

	auto& c = ECS::add_component<CComponent>(ent);
	REQUIRE(c.c == 0);
	ECS::update(game_clock);
	REQUIRE(b.b1 == 12);
	REQUIRE(b.b2 == 8);
	REQUIRE(c.c == 10);
}