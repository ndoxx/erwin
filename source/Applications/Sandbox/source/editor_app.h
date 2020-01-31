// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>
#include <map>

#define W_ENTRY_POINT
#include "erwin.h"
#include "layer_game.h"

using namespace erwin;
using namespace editor;

/*
	Test application to assess the level and asset editor for Erwin Engine
*/
class EditorSandbox: public Application
{
public:
	EditorSandbox() = default;
	~EditorSandbox() = default;

	virtual void on_pre_init() override;
	virtual void on_client_init() override;
	virtual void on_load() override;
	virtual void on_unload() override;
	virtual void on_imgui_render() override;
	bool on_keyboard_event(const KeyboardEvent& e);

private:
	GameLayer* game_layer_;
};

Application* erwin::create_application()
{
	return new EditorSandbox();
}
