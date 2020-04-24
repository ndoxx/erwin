#include "editor_app.h"

void ErwinEditor::on_pre_init()
{

}

void ErwinEditor::on_client_init()
{
	filesystem::set_asset_dir("source/Applications/Editor/assets");
	filesystem::set_client_config_dir("source/Applications/Editor/config");
	add_configuration("client.xml");
}

void ErwinEditor::on_load()
{
	EVENTBUS.subscribe(this, &ErwinEditor::on_keyboard_event);

    DLOG("application",1) << "Pushing game layer." << std::endl;
    push_layer(game_layer_ = new GameLayer());
}

void ErwinEditor::on_unload()
{

}

bool ErwinEditor::on_keyboard_event(const KeyboardEvent& e)
{
	// Terminate on Ctrl+ESCAPE
	if(e.pressed && e.key == keymap::WKEY::ESCAPE && (e.mods & keymap::WKEYMOD::CONTROL))
		EVENTBUS.publish(WindowCloseEvent());

	return false;
}

void ErwinEditor::on_imgui_render()
{

}
