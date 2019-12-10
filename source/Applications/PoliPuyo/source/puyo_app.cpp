// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>

#define W_ENTRY_POINT
#include "erwin.h"
#include "layer_2d.h"
#include "layer_presentation.h"
#include "layer_debug.h"
#include "game.h"

using namespace erwin;
using namespace puyo;

class Puyo: public Application
{
public:
	Puyo() = default;

	~Puyo()
	{
		Game::shutdown();
	}

	virtual void on_client_init() override
	{
		filesystem::set_asset_dir("source/Applications/PoliPuyo/assets");
		// filesystem::set_client_config_dir("source/Applications/PoliPuyo/config");
		// this->add_configuration("puyo.xml");
	}

	virtual void on_load() override
	{
		EVENTBUS.subscribe(this, &Puyo::on_keyboard_event);

		filesystem::set_asset_dir("source/Applications/PoliPuyo/assets");
		push_layer(new Layer2D());
		presentation_layer_ = new PresentationLayer();
		push_overlay(presentation_layer_);
		push_overlay(new DebugLayer());

		Game::init();
	}

	virtual void on_imgui_render() override
	{

	}

	bool on_keyboard_event(const KeyboardEvent& e)
	{
		// Terminate on ESCAPE
		if(e.pressed && e.key == keymap::WKEY::ESCAPE)
			EVENTBUS.publish(WindowCloseEvent());

		return false;
	}

private:
	PresentationLayer* presentation_layer_;
};

Application* erwin::create_application()
{
	return new Puyo();
}
