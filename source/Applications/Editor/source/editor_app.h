#define W_ENTRY_POINT
#include "erwin.h"

using namespace erwin;
using namespace editor;

namespace editor
{
	class SceneEditorLayer;
	class EditorBackgroundLayer;
	class ConsoleWidget;
	class KeybindingsWidget;
}
class GameLayer;

/*
	Editor application for Erwin Engine
*/
class ErwinEditor: public Application
{
public:
	ErwinEditor() = default;
	~ErwinEditor() = default;

	virtual void on_pre_init() override;
	virtual void on_client_init() override;
	virtual void on_load() override;
	virtual void on_unload() override;
	virtual void on_imgui_render() override;
	bool on_keyboard_event(const KeyboardEvent& e);

	void show_dockspace_window(bool* p_open);

private:
	GameLayer* game_layer_;
	SceneEditorLayer* scene_editor_layer_;
	EditorBackgroundLayer* editor_background_layer_;

	ConsoleWidget* console_;
	KeybindingsWidget* keybindings_widget_;

	bool exit_required_ = false;
	bool enable_docking_ = true;
};

Application* erwin::create_application()
{
	return new ErwinEditor();
}
