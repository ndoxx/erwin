#include "erwin.h"

class Sandbox: public erwin::Application
{
public:
	Sandbox()
	{

	}

	~Sandbox()
	{

	}
};

erwin::Application* erwin::create_application()
{
	return new Sandbox();
}
