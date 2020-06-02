#pragma once

#include "core/core.h"
#include <memory>

namespace erwin
{

class AbstractScene
{
public:
    virtual ~AbstractScene() = default;

    inline bool is_loaded() const { return loaded_; }
    inline void load() { loaded_ = on_load(); }
    inline void unload()
    {
        if(loaded_)
            on_unload();
        loaded_ = false;
    }
    // Cleanup all dead components and entities
    virtual void cleanup() = 0;

protected:
    // Load all needed data in graphics memory
    virtual bool on_load() = 0;
    // Unload all graphics resources
    virtual void on_unload() = 0;

protected:
    bool loaded_ = false;
};

class SceneManager
{
public:
    static void add_scene(hash_t name, std::unique_ptr<AbstractScene> scene);
    static void load_scene(hash_t name);
    static void unload_scene(hash_t name);
    static void remove_scene(hash_t name);
    static void make_current(hash_t name);
    static AbstractScene& get(hash_t name);
    static AbstractScene& get_current();
    static hash_t get_current_name();
    static bool has_current();

    template <typename SceneT> static inline SceneT& get_as(hash_t name) { return static_cast<SceneT&>(get(name)); }
    template <typename SceneT> static inline SceneT& get_current_as() { return static_cast<SceneT&>(get_current()); }

    template <typename SceneT, typename... ArgsT> static inline SceneT& create_scene(hash_t name, ArgsT&&... args)
    {
        add_scene(name, std::make_unique<SceneT>(std::forward<ArgsT>(args)...));
        return get_as<SceneT>(name);
    }
};

// Shorthand
namespace scn
{
template <typename SceneT> static inline SceneT& current() { return SceneManager::get_current_as<SceneT>(); }
[[maybe_unused]] static inline bool current_is_loaded() { return SceneManager::get_current().is_loaded(); }
} // namespace scn

} // namespace erwin