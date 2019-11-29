// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>

#define W_ENTRY_POINT
#include "erwin.h"
#include "layer_2d.h"
#include "layer_presentation.h"
#include "layer_debug.h"

using namespace erwin;


class Sandbox: public Application
{
public:
	Sandbox()
	{
		EVENTBUS.subscribe(this, &Sandbox::on_keyboard_event);

		filesystem::set_asset_dir("source/Applications/Sandbox/assets");
		push_layer(new Layer2D());
		push_overlay(new PresentationLayer());
		push_overlay(new DebugLayer());
	}

	~Sandbox() = default;

	bool on_keyboard_event(const KeyboardEvent& e)
	{
		// Terminate on ESCAPE
		if(e.pressed && e.key == keymap::WKEY::ESCAPE)
			EVENTBUS.publish(WindowCloseEvent());

		return false;
	}
};

Application* erwin::create_application()
{
	return new Sandbox();
}
