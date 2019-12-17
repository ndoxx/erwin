#include <array>
#include <random>
#include <vector>
#include <iostream>

#include "catch2/catch.hpp"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"
#include "event/event.h"
#include "event/event_bus.h"

using namespace erwin;

struct CollideEvent: public WEvent
{
	EVENT_DECLARATION(CollideEvent);

    CollideEvent() = default;
	CollideEvent(uint32_t first, uint32_t second):
	first(first),
	second(second)
	{

	}

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << first << " <-> " << second;
    }
#endif

    int first;
    int second;
};
EVENT_DEFINITION(CollideEvent);


class ColliderSystem
{
public:
	ColliderSystem()
	{

	}

	void fire_collision_instant(uint32_t first, uint32_t second)
	{
		EVENTBUS.publish(CollideEvent(first, second));
	}

	void enqueue_collision(uint32_t first, uint32_t second)
	{
		EVENTBUS.enqueue(new CollideEvent(first, second));
	}
};

class CollisionResponseSystem
{
public:
	CollisionResponseSystem()
	{

	}

	bool on_collision(const CollideEvent& event)
	{
		handled.push_back(std::pair(event.first, event.second));
		DLOG("event",1) << "Handled collision between " << event.first << " and " << event.second << std::endl;
		return false;
	}

	std::vector<std::pair<uint32_t,uint32_t>> handled;
};

class EventFixture
{
public:
	EventFixture():
	area(1_kB)
	{
        EventBus::init();
        EVENTBUS.init_event_pool<CollideEvent>(area, 8);
		EVENTBUS.subscribe(&collision_response, &CollisionResponseSystem::on_collision);
	}
	~EventFixture()
	{
        EVENTBUS.destroy_event_pool<CollideEvent>();
        EventBus::shutdown();
	}

protected:
    memory::HeapArea area;
    ColliderSystem collider;
    CollisionResponseSystem collision_response;
};

TEST_CASE_METHOD(EventFixture, "Firing event instantly", "[evt]")
{
	collider.fire_collision_instant(0,1);

	REQUIRE(collision_response.handled.size()==1);
	auto&& [a,b] = collision_response.handled[0];
	REQUIRE((a==0 && b==1));
}

TEST_CASE_METHOD(EventFixture, "Enqueueing event", "[evt]")
{
	collider.enqueue_collision(0,1);
	REQUIRE(collision_response.handled.size()==0);

	EVENTBUS.dispatch();

	REQUIRE(collision_response.handled.size()==1);
	auto&& [a,b] = collision_response.handled[0];
	REQUIRE((a==0 && b==1));
}

TEST_CASE_METHOD(EventFixture, "Enqueueing multiple events", "[evt]")
{
	collider.enqueue_collision(0,1);
	collider.enqueue_collision(2,3);
	REQUIRE(collision_response.handled.size()==0);

	EVENTBUS.dispatch();

	REQUIRE(collision_response.handled.size()==2);
	auto&& [a,b] = collision_response.handled[0];
	REQUIRE((a==0 && b==1));
	auto&& [c,d] = collision_response.handled[1];
	REQUIRE((c==2 && d==3));
}

/*
TEST_CASE_METHOD(EventFixture, "Saturating event pool", "[evt]")
{
	for(int ii=0; ii<10; ++ii)
		collider.enqueue_collision(0,1);

	EVENTBUS.dispatch();
	REQUIRE(collision_response.handled.size()==8);
}
*/