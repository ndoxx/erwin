#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
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

#include "memory/resource_pool.hpp"

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


class ABase
{
protected:
    int a_;
    float b_;

public:
    ABase(int a, float b):
    a_(a),
    b_(b)
    {}

    virtual ~ABase() = default;

    virtual void print() = 0;

    static ABase* create(PoolArena& arena, int a, float b);
};

class ADerived1: public ABase
{
public:
    ADerived1(int a, float b):
    ABase(a,b),
    c_(0),
    d_(42),
    e_(0)
    {}

    virtual ~ADerived1() = default;

    virtual void print() override
    {
        std::cout << "ADerived1 " << a_ << " " << b_ << " "
                  << c_ << " " << d_ << " " << e_ << std::endl;
    }

private:
    int c_;
    int d_;
    int e_;
};

class ADerived2: public ABase
{
public:
    ADerived2(int a, float b):
    ABase(a,b),
    c_(88)
    {}

    virtual ~ADerived2() = default;

    virtual void print() override
    {
        std::cout << "ADerived2 " << a_ << " " << b_ << " " << c_ << std::endl;
    }

private:
    int c_;
};

static int s_impl = 1;
ABase* ABase::create(PoolArena& arena, int a, float b)
{
    if(s_impl == 1)
        return W_NEW(ADerived1, arena)(a,b);
    else
        return W_NEW(ADerived2, arena)(a,b);
}

HANDLE_DECLARATION( AHandle );
HANDLE_DEFINITION( AHandle , 1 );

template <typename ResT, typename HandleT, size_t N>
void test_pool(ResourcePool<ResT, HandleT, N>& pool)
{
    std::vector<AHandle> handles;
    for(int ii=0; ii<10; ++ii)
    {
        AHandle ah = pool.acquire();
        pool.factory_create(ah, ii, ii*0.1f);
        std::cout << "hnd: " << ah.index << " -> ";
        pool[ah]->print();

        if(ii==3)
        {
            pool.destroy(ah);
            ah = pool.acquire();
            pool.factory_create(ah, 2*ii, 2*ii*0.1f);
            std::cout << "hnd: " << ah.index << " -> ";
            pool[ah]->print();
        }

        if(ii==7)
        {
            pool.destroy(handles[4]);
            handles[4] = pool.acquire();
            pool.factory_create(handles[4], 2*4, 2*4*0.1f);
            std::cout << "hnd: " << handles[4].index << " -> ";
            pool[handles[4]]->print();
        }

        handles.push_back(ah);
    }

    for(int ii=0; ii<10; ++ii)
        pool.destroy(handles[ii]);
}

int main(int argc, char** argv)
{
	init_logger();

    memory::HeapArea renderer_memory(1_MB);

    {
        size_t node_size = (s_impl==1) ? sizeof(ADerived1) : sizeof(ADerived2);
        ResourcePool<ABase, AHandle, 10> a_pool(renderer_memory, node_size, "ABase");
        test_pool(a_pool);
        a_pool.shutdown();
    }

    s_impl = 2;
    {
        size_t node_size = (s_impl==1) ? sizeof(ADerived1) : sizeof(ADerived2);
        ResourcePool<ABase, AHandle, 10> a_pool(renderer_memory, node_size, "ABase");
        test_pool(a_pool);
        a_pool.shutdown();
    }

	return 0;
}
