#include "erwin.h"
#include "event/window_events.h"

using namespace erwin;

class TestLayer: public Layer
{
public:
	TestLayer(const std::string& name):
	Layer("TestLayer_" + name)
	{

	}

	~TestLayer() {}

	virtual bool on_event(const MouseButtonEvent& event) override
	{
		DLOGN("event") << get_name() << " -> Handled event: " << event << std::endl;
		return true;
	}

protected:
	virtual void on_update() override
	{

	}
};

class Sandbox: public Application
{
public:
	Sandbox()
	{
		EVENTBUS.subscribe(this, &Sandbox::on_key_pressed_event);

		push_layer(new TestLayer("A"));
	}

	~Sandbox()
	{

	}

	bool on_key_pressed_event(const KeyboardEvent& e)
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
