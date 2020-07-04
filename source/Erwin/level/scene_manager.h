#pragma once

#include "core/core.h"
#include "level/scene.h"
#include <memory>

namespace erwin
{

class SceneManager
{
public:
    static void add_scene(hash_t name, std::unique_ptr<Scene> scene);
    static void load_scene(hash_t name);
    static void unload_scene(hash_t name);
    static void remove_scene(hash_t name);
    static void make_current(hash_t name);
    static Scene& get(hash_t name);
    static Scene& get_current();
    static hash_t get_current_name();
    static bool has_current();

    static inline Scene& create_scene(hash_t name)
    {
        add_scene(name, std::make_unique<Scene>());
        return get(name);
    }
};

// Shorthand
namespace scn
{
[[maybe_unused]] static inline Scene& current() { return SceneManager::get_current(); }
[[maybe_unused]] static bool current_is_loaded() { return SceneManager::get_current().is_loaded(); }
} // namespace scn

} // namespace erwin