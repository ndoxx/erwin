#include "editor_app.h"

void Editor::on_client_init()
{
	filesystem::set_asset_dir("source/Applications/Editor/assets");
	filesystem::set_client_config_dir("source/Applications/Editor/config");
	this->add_configuration("client.xml");
}

void Editor::on_load()
{
	EVENTBUS.subscribe(this, &Editor::on_keyboard_event);

    push_layer(layer_ = new LayerTest());
}

bool Editor::on_keyboard_event(const KeyboardEvent& e)
{
	// Terminate on ESCAPE
	if(e.pressed && e.key == keymap::WKEY::ESCAPE)
		EVENTBUS.publish(WindowCloseEvent());

	return false;
}

void Editor::on_imgui_render()
{

}
