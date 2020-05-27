#pragma once

#include "core/core.h"
#include <memory>

namespace erwin
{

class AbstractScene
{
public:
    virtual ~AbstractScene() = default;

    // Load all needed data in graphics memory
    virtual void load() = 0;
    // Unload all graphics resources
    virtual void unload() = 0;
    // Cleanup all dead components and entities
    virtual void cleanup() = 0;
};

class SceneManager
{
public:
    static void add_scene(hash_t name, std::unique_ptr<AbstractScene> scene);
    static void load_scene(hash_t name);
    static void remove_scene(hash_t name);
    static void make_current(hash_t name);
    static AbstractScene& get(hash_t name);
    static AbstractScene& get_current();

    template <typename SceneT> static inline SceneT& get_as(hash_t name) { return static_cast<SceneT&>(get(name)); }
    template <typename SceneT> static inline SceneT& get_current_as()    { return static_cast<SceneT&>(get_current()); }
};

// Shorthand
namespace scn
{
	template <typename SceneT> static inline SceneT& current() { return SceneManager::get_current_as<SceneT>(); }
} // namespace scn

} // namespace erwin