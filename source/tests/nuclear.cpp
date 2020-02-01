#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <bitset>
#include <atomic>

#include "glm/glm.hpp"
#include "memory/memory.hpp"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"

// #include "math/convolution.h"
// #include "render/main_renderer.h"

#include "input/input.h"
#include "filesystem/filesystem.h"


using namespace erwin;

void init_logger()
{
    WLOGGER(create_channel("memory", 3));
    WLOGGER(create_channel("thread", 3));
	WLOGGER(create_channel("nuclear", 3));
	WLOGGER(create_channel("entity", 3));
	WLOGGER(create_channel("config", 3));
	WLOGGER(attach_all("console_sink", std::make_unique<dbg::ConsoleSink>()));
    WLOGGER(set_single_threaded(true));
    WLOGGER(set_backtrace_on_error(false));
    WLOGGER(spawn());
    WLOGGER(sync());

    DLOGN("nuclear") << "Nuclear test" << std::endl;
}

int main(int argc, char** argv)
{
	init_logger();

	int N = 6;
	for(int ii=0; ii<N; ++ii)
	{
		float alpha = 2*M_PI*(ii/float(N));
		DLOGR("nuclear") << "vec2(" << cos(alpha) << "," << sin(alpha) << ")," << std::endl;
	}

	return 0;
}


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
