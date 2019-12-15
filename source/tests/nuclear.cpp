#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <bitset>

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

#include "utils/random.h"

using namespace erwin;

void init_logger()
{
    WLOGGER.create_channel("memory", 3);
	WLOGGER.create_channel("nuclear", 3);
	WLOGGER.create_channel("entity", 3);
	WLOGGER.attach_all("console_sink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.set_single_threaded(true);
    WLOGGER.set_backtrace_on_error(false);
    WLOGGER.spawn();
    WLOGGER.sync();

    DLOGN("nuclear") << "Nuclear test" << std::endl;
}
/*
int main(int argc, char** argv)
{
	init_logger();

	XorShiftEngine rng;
	rng.seed();
	// rng.seed(0);
	// rng.seed({123456789, 789456123});
	// rng.seed_string("123456789:789456123");

	auto seed = rng.get_seed();
	DLOG("nuclear",1) << "Seed: " << seed << std::endl;

	for(int ii=0; ii<10; ++ii)
		DLOG("nuclear",1) << rng() << " ";
	DLOG("nuclear",1) << std::endl;

	std::uniform_int_distribution<int> dis(1,6);
	for(int ii=0; ii<100; ++ii)
		DLOG("nuclear",1) << dis(rng) << " ";
	DLOG("nuclear",1) << std::endl;

	std::uniform_real_distribution<float> fdis(-1.f,1.f);
	for(int ii=0; ii<10; ++ii)
		DLOG("nuclear",1) << fdis(rng) << " ";
	DLOG("nuclear",1) << std::endl;

	int Nexp = 1000000;
	std::array<uint32_t,64> bit_stats;
	for(int ii=0; ii<64; ++ii)
		bit_stats[ii] = 0;
	for(int kk=0;kk<Nexp; ++kk)
	{
		uint64_t rnd = rng.rand64();
		for(int ii=0; ii<64; ++ii)
			bit_stats[ii] += uint32_t((rnd & (1ul<<ii))!=0);
	}

	for(int ii=0; ii<64; ++ii)
		DLOG("nuclear",1) << ii << ": " << bit_stats[ii]/float(Nexp) << std::endl;


	return 0;
}
*/

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

int main(int argc, char** argv)
{
	init_logger();

	size_t N = 32;

	memory::HeapArea area(1_MB);

	EntityManager mgr;
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
	for(int ii=0; ii<5; ++ii)
	{
		EntityID ent = mgr.create_entity();
		mgr.create_component<BComponent>(ent);
		mgr.create_component<CComponent>(ent);
		mgr.submit_entity(ent);
	}
	// Entities that contain A and C components (should be ignored)
	for(int ii=0; ii<4; ++ii)
	{
		EntityID ent = mgr.create_entity();
		mgr.create_component<AComponent>(ent);
		mgr.create_component<CComponent>(ent);
		mgr.submit_entity(ent);
	}

	GameClock game_clock;
	mgr.update(game_clock);
	mgr.destroy_entity(0);
	mgr.update(game_clock);

	return 0;
}


/*
class DummyComponent: public Component
{
public:
	ID_DECLARATION(DummyComponent);
	COMPONENT_DECLARATION(DummyComponent);

	uint32_t a;
	uint32_t b;
	uint64_t c;

	DummyComponent(): a(0xB16B00B5), b(0x42424242), c(0xD0D0DADAD0D0DADA) {}
	virtual bool init(void* description) override final { return true; }
};
COMPONENT_DEFINITION(DummyComponent);

int main(int argc, char** argv)
{
	init_logger();

    memory::hex_dump_highlight(0xf0f0f0f0, WCB(100,0,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,0,100));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(200,100,0));

    DLOG("nuclear",1) << "Pool arena decoration size: " << PoolArena::DECORATION_SIZE << "_B" << std::endl;
    DLOG("nuclear",1) << "Dummy component size: " << sizeof(DummyComponent) << "_B" << std::endl;

	memory::HeapArea area(1_MB);
	size_t decorated_size = sizeof(DummyComponent) + PoolArena::DECORATION_SIZE;
	size_t n_dummy = 5;
	size_t pool_size = n_dummy * decorated_size;

	DummyComponent::init_pool(area.require_pool_block<PoolArena>(sizeof(DummyComponent), n_dummy), n_dummy);

	std::vector<Component*> components;
	for(int ii=0; ii<n_dummy; ++ii)
		components.push_back(new DummyComponent());

    memory::hex_dump(std::cout, DummyComponent::s_ppool_->get_allocator().begin(), pool_size, "HEX DUMP");

	delete components[2];
	delete components[3];

	DummyComponent* pdc3 = new DummyComponent();
	DummyComponent* pdc2 = new DummyComponent();
	pdc3->a = 0x11111111;
	pdc3->b = 0x22222222;
	pdc3->c = 0x3333333344444444;
	pdc2->a = 0x55555555;
	pdc2->b = 0x66666666;
	pdc2->c = 0x7777777788888888;
	components[3] = pdc3;
	components[2] = pdc2;

    memory::hex_dump(std::cout, DummyComponent::s_ppool_->get_allocator().begin(), pool_size, "HEX DUMP");
	for(auto* pcmp: components)
		delete pcmp;
    memory::hex_dump(std::cout, DummyComponent::s_ppool_->get_allocator().begin(), pool_size, "HEX DUMP");

	DummyComponent::destroy_pool();

	return 0;
}
*/
