#include <array>
#include <random>
#include <vector>
#include <iostream>

#define W_TEST
#include "catch2/catch.hpp"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"
#include "memory/memory.hpp"
#include "event/event_bus.h"
#include "event/event.h"

using namespace erwin;

struct CollideEvent
{
	EVENT_DECLARATIONS(CollideEvent);

    CollideEvent() = default;
	CollideEvent(uint32_t first, uint32_t second):
	first(first),
	second(second)
	{

	}

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const CollideEvent& rhs)
    {
        stream << rhs.first << " <-> " << rhs.second;
        return stream;
    }
#endif

    int first;
    int second;
};


class ColliderSystem
{
public:
	ColliderSystem()
	{

	}

	void fire_collision_instant(uint32_t first, uint32_t second)
	{
		EventBus::fire(CollideEvent(first, second));
	}

	void enqueue_collision(uint32_t first, uint32_t second)
	{
		EventBus::enqueue(CollideEvent(first, second));
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
	EventFixture()
	{
		EventBus::subscribe(&collision_response, &CollisionResponseSystem::on_collision);
	}
	~EventFixture()
	{
		EventBus::reset();
	}

protected:
    ColliderSystem collider;
    CollisionResponseSystem collision_response;
};

TEST_CASE_METHOD(EventFixture, "Firing event instantly", "[evt]")
{
	collider.fire_collision_instant(0,1);

	REQUIRE(EventBus::empty());
	REQUIRE(collision_response.handled.size()==1);
	auto&& [a,b] = collision_response.handled[0];
	REQUIRE((a==0 && b==1));
}

TEST_CASE_METHOD(EventFixture, "Enqueueing event", "[evt]")
{
	collider.enqueue_collision(0,1);
	REQUIRE(collision_response.handled.size()==0);

	EventBus::dispatch();

	REQUIRE(collision_response.handled.size()==1);
	auto&& [a,b] = collision_response.handled[0];
	REQUIRE((a==0 && b==1));
}

TEST_CASE_METHOD(EventFixture, "Enqueueing multiple events", "[evt]")
{
	collider.enqueue_collision(0,1);
	collider.enqueue_collision(2,3);
	REQUIRE(collision_response.handled.size()==0);

	EventBus::dispatch();

	REQUIRE(collision_response.handled.size()==2);
	auto&& [a,b] = collision_response.handled[0];
	REQUIRE((a==0 && b==1));
	auto&& [c,d] = collision_response.handled[1];
	REQUIRE((c==2 && d==3));
}
