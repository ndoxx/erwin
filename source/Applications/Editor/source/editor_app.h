#define W_ENTRY_POINT
#include "erwin.h"
#include "layer/gui_layer.h"

using namespace erwin;
using namespace editor;

namespace editor
{
	class SceneEditorLayer;
	class EditorBackgroundLayer;
	class ConsoleWidget;
	class KeybindingsWidget;
}
class SceneViewLayer;

/*
	Allows to group multiple layers together and batch enable/disable them
*/
struct EditorState
{
	std::vector<erwin::Layer*> layers;
	editor::GuiLayer* gui_layer = nullptr;

	inline void enable(bool value=true)
	{
		for(auto* layer: layers)
			layer->set_enabled(value);
		if(gui_layer)
			gui_layer->set_enabled(value);
	}
};

enum class EditorStateIdx: size_t
{
	SCENE_EDITION = 0,

	COUNT
};

/*
	Editor application for Erwin Engine
*/
class ErwinEditor: public Application
{
public:
	virtual void on_pre_init() override;
	virtual void on_client_init() override;
	virtual void on_load() override;
	virtual void on_unload() override;
	virtual void on_imgui_render() override;
	bool on_keyboard_event(const KeyboardEvent& e);

	void show_dockspace_window(bool* p_open);

	void create_state(EditorStateIdx idx, EditorState&& state);
	void switch_state(EditorStateIdx idx);

private:
	std::array<EditorState, size_t(EditorStateIdx::COUNT)> states_;
	EditorStateIdx current_state_idx_ = EditorStateIdx::SCENE_EDITION;

	ConsoleWidget* console_ = nullptr;
	KeybindingsWidget* keybindings_widget_ = nullptr;

	bool exit_required_ = false;
	bool enable_docking_ = true;
};

Application* erwin::create_application()
{
	return new ErwinEditor();
}
