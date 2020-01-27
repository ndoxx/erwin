#include "editor_app.h"

void EditorSandbox::on_pre_init()
{

}

void EditorSandbox::on_client_init()
{
	filesystem::set_asset_dir("source/Applications/Editor/assets");
	filesystem::set_client_config_dir("source/Applications/Editor/config");
	this->add_configuration("client.xml");
}

void EditorSandbox::on_load()
{
	EVENTBUS.subscribe(this, &EditorSandbox::on_keyboard_event);

    DLOG("application",1) << "Pushing game layer." << std::endl;
    push_layer(game_layer_ = new GameLayer(get_client_area()));
}

void EditorSandbox::on_unload()
{

}

bool EditorSandbox::on_keyboard_event(const KeyboardEvent& e)
{
	// Terminate on ESCAPE
	if(e.pressed && e.key == keymap::WKEY::ESCAPE)
		EVENTBUS.publish(WindowCloseEvent());

	return false;
}

void EditorSandbox::on_imgui_render()
{

}
