// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>
#include <map>

#define W_ENTRY_POINT
#include "erwin.h"
#include "layer_game.h"
#include "editor/layer_editor.h"
#include "editor/widget.h"

using namespace erwin;
using namespace editor;

/*
	Level and asset editor for Erwin Engine
*/
class Editor: public Application
{
public:
	Editor() = default;
	~Editor() = default;

	virtual void on_pre_init() override;
	virtual void on_client_init() override;
	virtual void on_load() override;
	virtual void on_unload() override;
	virtual void on_imgui_render() override;
	bool on_keyboard_event(const KeyboardEvent& e);

private:
	FramebufferHandle game_view_fb_;
	erwin::Scene scene_;
	erwin::EntityManager entity_manager_;

	GameLayer* game_layer_;
	EditorLayer* editor_layer_;

	Widget* console_;
};

Application* erwin::create_application()
{
	return new Editor();
}
