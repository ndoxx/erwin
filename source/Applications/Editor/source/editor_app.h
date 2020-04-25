// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>
#include <map>

#define W_ENTRY_POINT
#include "erwin.h"

using namespace erwin;
using namespace editor;

namespace editor
{
	class EditorLayer;
	class EditorBackgroundLayer;
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

private:
	GameLayer* game_layer_;
	EditorLayer* editor_layer_;
	EditorBackgroundLayer* editor_background_layer_;
};

Application* erwin::create_application()
{
	return new ErwinEditor();
}
