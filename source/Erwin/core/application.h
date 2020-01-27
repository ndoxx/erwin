#pragma once

#include <memory>

#include "core/core.h"
#include "core/window.h"
#include "core/layer_stack.h"
#include "core/game_clock.h"
#include "filesystem/filesystem.h"
#include "entity/entity_manager.h"
#include "editor/scene.h" // MOVE

namespace editor
{
	class EditorLayer;
}

namespace erwin
{

class Scene;
class W_API Application
{
public:
	Application();
	virtual ~Application();

	virtual void on_pre_init() { }
	virtual void on_client_init() { }
	virtual void on_load() { }
	virtual void on_unload() { }
	virtual void on_imgui_render() { }

	size_t push_layer(Layer* layer);
	size_t push_overlay(Layer* layer);

	// Create an editor overlay and push it to the layer stack (defined in editor/layer_editor.cpp)
	void build_editor();

	inline void set_layer_enabled(size_t index, bool value) { layer_stack_.set_layer_enabled(index, value); }
	void toggle_imgui_layer();

	// Add an XML configuration file to be parsed at the end of init()
	void add_configuration(const std::string& filename);

	bool init();
	void run();

	memory::HeapArea& get_client_area();
	const memory::HeapArea& get_system_area() const;
	const memory::HeapArea& get_render_area() const;

	static inline Application& get_instance() { return *pinstance_; }
	inline const Window& get_window() { return *window_; }

	bool on_window_close_event(const WindowCloseEvent& e);

	static inline EntityManager& ECS() { return s_ECS; }
	static inline Scene& SCENE() { return s_SCENE; }

protected:
	editor::EditorLayer* EDITOR_LAYER;
	static EntityManager s_ECS;
	static Scene s_SCENE;

private:
	static Application* pinstance_;
	WScope<Window> window_;
	bool is_running_;
	bool minimized_;

	LayerStack layer_stack_;
	GameClock game_clock_;
};

// Defined in the client
Application* create_application();

} // namespace erwin