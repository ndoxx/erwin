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
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"

#include "entity/component.h"
#include "entity/component_system.h"
#include "entity/entity_manager.h"
#include "core/game_clock.h"
#include "core/handle.h"

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



HANDLE_DECLARATION( FooHandle );
HANDLE_DEFINITION( FooHandle );

HANDLE_DECLARATION( BarHandle );
HANDLE_DEFINITION( BarHandle );

template <typename HandleT>
void display(const std::vector<HandleT>& handle_vec)
{
	for(auto&& hnd: handle_vec)
		std::cout << hnd.index << " ";
	std::cout << std::endl;
}

int main(int argc, char** argv)
{
	init_logger();

	memory::HeapArea area(8_kB);
	LinearArena handle_arena(area.require_block(2_kB));

	FooHandle::init_pool(handle_arena);
	BarHandle::init_pool(handle_arena);

	std::vector<FooHandle> foo_handles;
	std::vector<BarHandle> bar_handles;

	for(int ii=0; ii<8; ++ii)
		foo_handles.push_back(FooHandle::acquire());

	display(foo_handles);
	foo_handles[1].release(); foo_handles[1] = std::move(foo_handles.back()); foo_handles.pop_back();
	foo_handles[2].release(); foo_handles[2] = std::move(foo_handles.back()); foo_handles.pop_back();
	foo_handles[5].release(); foo_handles[5] = std::move(foo_handles.back()); foo_handles.pop_back();
	display(foo_handles);

	for(int ii=0; ii<15; ++ii)
		bar_handles.push_back(BarHandle::acquire());

	for(int ii=0; ii<8; ++ii)
		foo_handles.push_back(FooHandle::acquire());

	display(foo_handles);
	display(bar_handles);


	FooHandle::destroy_pool(handle_arena);
	BarHandle::destroy_pool(handle_arena);

	return 0;
}

/*
struct Foo
{
	uint32_t a = 0x42424242;
	uint32_t b = 0x69696969;
	uint64_t c = 0xd1d1dadad1d1d3d3;
};

typedef memory::MemoryArena<memory::PoolAllocator, 
		    				memory::policy::SingleThread, 
		    				memory::policy::SimpleBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::SimpleMemoryTracking> MyPoolArena;

// typedef memory::MemoryArena<memory::PoolAllocator, 
// 		    				memory::policy::SingleThread, 
// 		    				memory::policy::NoBoundsChecking,
// 		    				memory::policy::NoMemoryTagging,
// 		    				memory::policy::NoMemoryTracking> MyPoolArena;

int main(int argc, char** argv)
{
	init_logger();

    memory::hex_dump_highlight(0xf0f0f0f0, WCB(200,50,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,50,200));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(50,120,0));

    uint32_t dump_size = 512_B;

    uint32_t max_foo = 8;
    uint32_t max_align = 16-1;
    uint32_t node_size = sizeof(Foo) + max_align;

	memory::HeapArea area(8_kB);
	area.fill(0xaa);
	MyPoolArena pool(area.require_pool_block<MyPoolArena>(node_size, max_foo), node_size, max_foo, MyPoolArena::DECORATION_SIZE);

	std::vector<Foo*> foos;

	area.debug_hex_dump(std::cout, dump_size);
	for(int kk=0; kk<2; ++kk)
	{
		for(int ii=0; ii<8; ++ii)
		{
			Foo* foo = W_NEW_ALIGN(Foo, pool, 8);
			foos.push_back(foo);
		}
		area.debug_hex_dump(std::cout, dump_size);

		W_DELETE(foos[2], pool); foos[2] = std::move(foos.back()); foos.pop_back();
		W_DELETE(foos[5], pool); foos[5] = std::move(foos.back()); foos.pop_back();
		foos.push_back(W_NEW_ALIGN(Foo, pool, 16));
		foos.push_back(W_NEW(Foo, pool));
		area.debug_hex_dump(std::cout, dump_size);

		for(Foo* foo: foos)
		{
			W_DELETE(foo, pool);
		}
		area.debug_hex_dump(std::cout, dump_size);

		foos.clear();
	}

	return 0;
}
*/

/*
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

    memory::hex_dump_highlight(0xf0f0f0f0, WCB(200,50,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,50,200));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(50,120,0));

	size_t N = 8;

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
	for(int ii=0; ii<4; ++ii)
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
	area.debug_hex_dump(std::cout);
	mgr.destroy_entity(0);
	mgr.update(game_clock);
	area.debug_hex_dump(std::cout);

	return 0;
}
*/
