#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <bitset>
#include <atomic>

#include "glm/glm.hpp"
#include "memory/memory.hpp"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"

#include "memory/resource_pool.hpp"
#include "core/window.h"
#include "render/buffer.h"
#include "render/handles.h"

using namespace erwin;

void init_logger()
{
    WLOGGER(create_channel("memory", 3));
    WLOGGER(create_channel("thread", 3));
	WLOGGER(create_channel("nuclear", 3));
	WLOGGER(create_channel("entity", 3));
    WLOGGER(create_channel("config", 3));
	WLOGGER(create_channel("render", 3));
	WLOGGER(attach_all("console_sink", std::make_unique<dbg::ConsoleSink>()));
    WLOGGER(set_single_threaded(true));
    WLOGGER(set_backtrace_on_error(false));
    WLOGGER(spawn());
    WLOGGER(sync());

    DLOGN("nuclear") << "Nuclear test" << std::endl;
}

/*
class BufBase {};
class BufDerived1: virtual BufBase {};
class BufDerived2: virtual BufBase {};

class ABase: public virtual BufBase
{
protected:
    int a_;
    float b_;
    BufferLayout layout_;

public:
    ABase(int a, float b):
    a_(a),
    b_(b)
    {}

    virtual ~ABase() = default;

    virtual void print() = 0;

    static ABase* create(PoolArena& arena, int a, float b);
};

class ADerived1: public BufDerived1, public ABase
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

class ADerived2: public BufDerived2, public ABase
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
    {
        return W_NEW(ADerived1, arena)(a,b);
        // return (ABase*)(void*)W_NEW(ADerived1, arena)(a,b); // Seems to work. WHY?
    }
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

    memory::hex_dump_highlight(0xf0f0f0f0, WCB(200,50,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,50,200));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(50,120,0));

    #ifdef ARENA_RETAIL
        std::cout << "Memory arena: RETAIL build" << std::endl;
    #endif

    memory::HeapArea renderer_memory(1_MB);

#if 1
    ResourcePool<ABase, AHandle, 10> a_pool(renderer_memory, sizeof(ADerived1), "As");

    AHandle ah = a_pool.acquire();
    memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(renderer_memory.begin()), 256_B);
    auto* pa = a_pool.factory_create(ah, 42, 42.42f);
    pa->print();
    memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(renderer_memory.begin()), 256_B);
    a_pool.destroy(ah);
    memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(renderer_memory.begin()), 256_B);

    // test_pool(a_pool);

    a_pool.shutdown();
    return 0;

#else

    WindowProps props
    {
        "ErwinEngine",
        1280,
        1024,
        false,
        false,
        true,
        true
    };
    auto window = Window::create(props);

    ResourcePool<VertexBuffer, VertexBufferHandle, 10> vbos(renderer_memory, VertexBuffer::node_size(), "VertexBuffers");

    uint32_t data[12] = {0,1,2,2,1,3,3,4,5,6,7,8};
    float fdata[12]   = {0.f,1.f,2.f,2.f,1.f,3.f,3.f,4.f,5.f,6.f,7.f,8.f};
    BufferLayout layout{{"a_position"_h, ShaderDataType::Vec3}};
    VertexBufferHandle vbo = vbos.acquire();
    memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(renderer_memory.begin()), 256_B);
    VertexBuffer* p_vbo = vbos.factory_create(vbo, fdata, 12, layout);
    DLOGW("memory") << "Factory returned: " << std::hex << uint64_t((void*)p_vbo) << std::endl;
    memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(renderer_memory.begin()), 256_B);
    vbos.destroy(vbo);
    memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(renderer_memory.begin()), 256_B);

    vbos.shutdown();

    return 0;
#endif
}
*/


int main(int argc, char** argv)
{
    init_logger();

    memory::hex_dump_highlight(0xf0f0f0f0, WCB(200,50,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,50,200));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(50,120,0));

    #define ICON_FA_GLASS u8"\uf000"

    std::cout << ICON_FA_GLASS << std::endl;

    printf("Plop: %s", ICON_FA_GLASS);

    return 0;
}