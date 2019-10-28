// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>

#define W_ENTRY_POINT
#include "erwin.h"

// #include "layer_qtest_2d.h"
#include "layer_test.h"

using namespace erwin;


class Sandbox: public Application
{
public:
	Sandbox()
	{
		EVENTBUS.subscribe(this, &Sandbox::on_keyboard_event);

		filesystem::set_asset_dir("source/Applications/Sandbox/assets");
		// push_layer(new LayerQTest2D());
		push_layer(new LayerTest());
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
