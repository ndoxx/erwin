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

private:
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
		push_overlay(new TestLayer("O1"));
		push_layer(new TestLayer("B"));
		push_overlay(new TestLayer("O2"));
		push_overlay(new TestLayer("O3"));
		push_layer(new TestLayer("C"));
		push_layer(new TestLayer("D"));
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
