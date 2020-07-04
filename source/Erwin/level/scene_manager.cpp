#include <map>
#include <optional>

#include "level/scene_manager.h"
#include "level/scene.h"
#include "core/intern_string.h"
#include "debug/logger.h"

namespace erwin
{

static struct
{
	std::map<hash_t, std::unique_ptr<Scene>> scenes;
	std::optional<std::reference_wrapper<Scene>> current_scene;
	hash_t current_scene_name = 0;
} s_storage;

void SceneManager::add_scene(hash_t name, std::unique_ptr<Scene> scene)
{
	W_ASSERT_FMT(s_storage.scenes.find(name) == s_storage.scenes.end(), "Cannot add a new scene at this name, name \"%s\" is already in use.", istr::resolve(name).c_str());

	DLOGN("application") << "Adding new scene: " << WCC('n') << istr::resolve(name) << std::endl;
	s_storage.scenes[name] = std::move(scene);
}

void SceneManager::unload_scene(hash_t name)
{
	auto it = s_storage.scenes.find(name);
	W_ASSERT_FMT(it != s_storage.scenes.end(), "Cannot find scene at this name: \"%s\".", istr::resolve(name).c_str());
	
	DLOGN("application") << "Unloading scene: " << WCC('n') << istr::resolve(name) << std::endl;
	it->second->unload();
}

Scene& SceneManager::get(hash_t name)
{
	auto it = s_storage.scenes.find(name);
	W_ASSERT_FMT(it != s_storage.scenes.end(), "Cannot find scene at this name: \"%s\".", istr::resolve(name).c_str());

	return *it->second;
}

Scene& SceneManager::get_current()
{
	W_ASSERT(s_storage.current_scene.has_value(), "No current scene is set.");
	return s_storage.current_scene->get();
}

hash_t SceneManager::get_current_name()
{
	return s_storage.current_scene_name;
}

bool SceneManager::has_current()
{
	return s_storage.current_scene.has_value();
}

void SceneManager::remove_scene(hash_t name)
{
	auto it = s_storage.scenes.find(name);
	W_ASSERT_FMT(it != s_storage.scenes.end(), "Cannot find scene at this name: \"%s\".", istr::resolve(name).c_str());
	
	DLOGN("application") << "Removing scene: " << WCC('n') << istr::resolve(name) << std::endl;
	it->second->unload();
	s_storage.scenes.erase(it);
}

void SceneManager::make_current(hash_t name)
{
	auto it = s_storage.scenes.find(name);
	W_ASSERT_FMT(it != s_storage.scenes.end(), "Cannot find scene at this name: \"%s\".", istr::resolve(name).c_str());

	s_storage.current_scene = *it->second;
	s_storage.current_scene_name = name;
}

} // namespace erwin