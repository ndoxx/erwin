#include "rtest_app.h"

void RTest::on_client_init()
{
	filesystem::set_asset_dir("source/Applications/RTest/assets");
	filesystem::set_client_config_dir("source/Applications/RTest/config");
	this->add_configuration("client.xml");
}

void RTest::on_load()
{
	EventBus::subscribe(this, &RTest::on_keyboard_event);

    push_layer(layer_ = new LayerTest());
}

bool RTest::on_keyboard_event(const KeyboardEvent& e)
{
	// Terminate on ESCAPE
	if(e.pressed && e.key == keymap::WKEY::ESCAPE)
		EventBus::enqueue(WindowCloseEvent());

	return false;
}

void RTest::on_imgui_render()
{

}
