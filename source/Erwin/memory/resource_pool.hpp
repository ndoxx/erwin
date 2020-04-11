#pragma once

#include "memory/arena.h"
#include "memory/handle_pool.h"
#include "core/handle.h"
#include <array>

namespace erwin
{

template<typename ResT, typename HandleT, size_t MAX_NODES>
class ResourcePool
{
public:
    static constexpr std::size_t k_handle_alloc_size = 2 * sizeof(HandlePoolT<MAX_NODES>);

    ResourcePool():
    initialized_(false)
    {}

    ResourcePool(memory::HeapArea& area, size_t node_size, const std::string& debug_name="")
    {
        init(area, node_size, debug_name);
    }

    ~ResourcePool()
    {
        if(initialized_)
            shutdown();
    }

    inline ResT* operator[](HandleT h) { return objects_[h.index]; }

    inline HandleT acquire()
    {
        HandleT h = HandleT::acquire();
        W_ASSERT_FMT(h.is_valid(), "No more free handle in handle pool %s.", debug_name_.c_str());
        return h;
    }

    // ASSUME: ResT has a static factory member create() that takes a
    // PoolArena& as a first argument, and several other arguments
    // forwarded to the constructor
    template<typename... Args>
    inline ResT* factory_create(HandleT h, Args&&... args)
    {
        ResT* obj = ResT::create(pool_, std::forward<Args>(args)...);
        objects_[h.index] = obj;
        return obj;
    }

    template<typename... Args>
    inline ResT* create(HandleT h, Args&&... args)
    {
        ResT* obj = W_NEW(ResT, pool_)(std::forward<Args>(args)...);
        objects_[h.index] = obj;
        return obj;
    }

    inline void destroy(HandleT h)
    {
        if(objects_[h.index])
        {
            W_DELETE(objects_[h.index], pool_);
            objects_[h.index] = nullptr;
            h.release();
        }
    }

    void init(memory::HeapArea& area, size_t node_size, const std::string& debug_name="")
    {
        debug_name_ = debug_name;
        // Init handle pool
        W_ASSERT(HandleT::s_ppool_==nullptr, "Memory pool is already initialized.");
        std::string handle_arena_name = "HND_" + debug_name;
        handle_arena_.init(area, k_handle_alloc_size, handle_arena_name.c_str());
        HandleT::s_ppool_ = W_NEW(HandlePoolT<MAX_NODES>, handle_arena_);
        // Init object pool
        pool_.init(area, node_size + PoolArena::DECORATION_SIZE, MAX_NODES, debug_name.c_str());
        objects_.fill(nullptr);
        initialized_ = true;
    }

    void shutdown()
    {
        // Destroy handle pool
        HandleT::destroy_pool(handle_arena_);
        // Destroy objects
        for(ResT* obj: objects_)
            if(obj)
                W_DELETE(obj, pool_);
        objects_.fill(nullptr);
        pool_.shutdown();
        initialized_ = false;
    }

private:
    bool initialized_;
    LinearArena handle_arena_;
    PoolArena pool_;
    std::array<ResT*,MAX_NODES> objects_;
    std::string debug_name_;
};

} // namespace erwin