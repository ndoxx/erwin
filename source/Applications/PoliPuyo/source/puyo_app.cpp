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
	Puyo()
	{
		EVENTBUS.subscribe(this, &Puyo::on_keyboard_event);

		filesystem::set_asset_dir("source/Applications/PoliPuyo/assets");
		push_layer(new Layer2D());
		presentation_layer_ = new PresentationLayer();
		push_overlay(presentation_layer_);
		push_overlay(new DebugLayer());

		Game::init();
	}

	~Puyo()
	{
		Game::shutdown();
	}

	bool on_keyboard_event(const KeyboardEvent& e)
	{
		// Terminate on ESCAPE
		if(e.pressed && e.key == keymap::WKEY::ESCAPE)
			EVENTBUS.publish(WindowCloseEvent());

		return false;
	}

	void on_imgui_render()
	{

	}

private:
	PresentationLayer* presentation_layer_;
};

Application* erwin::create_application()
{
	return new Puyo();
}
