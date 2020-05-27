#include <map>
#include <optional>

#include "level/scene_manager.h"
#include "core/intern_string.h"
#include "debug/logger.h"

namespace erwin
{

static struct
{
	std::map<hash_t, std::unique_ptr<AbstractScene>> scenes;
	std::optional<std::reference_wrapper<AbstractScene>> current_scene;
} s_storage;

void SceneManager::add_scene(hash_t name, std::unique_ptr<AbstractScene> scene)
{
	W_ASSERT_FMT(s_storage.scenes.find(name) == s_storage.scenes.end(), "Cannot add a new scene at this name, name \"%s\" is already in use.", istr::resolve(name).c_str());

	DLOGN("application") << "Adding new scene: " << WCC('n') << istr::resolve(name) << std::endl;
	s_storage.scenes[name] = std::move(scene);
}

void SceneManager::load_scene(hash_t name)
{
	auto it = s_storage.scenes.find(name);
	W_ASSERT_FMT(it != s_storage.scenes.end(), "Cannot find scene at this name: \"%s\".", istr::resolve(name).c_str());
	
	DLOGN("application") << "Loading scene: " << WCC('n') << istr::resolve(name) << std::endl;
	it->second->load();
}

AbstractScene& SceneManager::get(hash_t name)
{
	auto it = s_storage.scenes.find(name);
	W_ASSERT_FMT(it != s_storage.scenes.end(), "Cannot find scene at this name: \"%s\".", istr::resolve(name).c_str());

	return *it->second;
}

AbstractScene& SceneManager::get_current()
{
	W_ASSERT(s_storage.current_scene.has_value(), "No current scene is set.");
	return s_storage.current_scene->get();
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
}


} // namespace erwin