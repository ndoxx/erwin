#include "erwin.h"
#include "event/window_events.h"

class Sandbox: public erwin::Application
{
public:
	Sandbox()
	{
		erwin::EVENTBUS.subscribe(this, &Sandbox::on_key_pressed_event);
	}

	~Sandbox()
	{

	}

	bool on_key_pressed_event(const erwin::KeyPressedEvent& e)
	{
		// Terminate on ESCAPE
		if(e.key == erwin::keymap::WKEY::ESCAPE)
			erwin::EVENTBUS.publish(erwin::WindowCloseEvent());

		return false;
	}
};

erwin::Application* erwin::create_application()
{
	return new Sandbox();
}
