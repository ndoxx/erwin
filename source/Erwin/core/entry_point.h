#pragma once

// Client application just has to inherit a class from Application
// and define a create_application() function that returns a derived instance.
// The lib handles the definition of main() here.
#include "core/application.h"

namespace erwin
{
	extern Application* create_application();
}

int main(int argc, char** argv)
{
	W_PROFILE_BEGIN_SESSION("startup", "wprofile-startup.json");
	auto app = erwin::create_application();
	if(!app->init())
	{
		delete app;
		return -1;
	}
	W_PROFILE_END_SESSION();

	W_PROFILE_BEGIN_SESSION("runtime", "wprofile-runtime.json");
	app->run();
	W_PROFILE_END_SESSION();

	W_PROFILE_BEGIN_SESSION("shutdown", "wprofile-shutdown.json");
	delete app;
	W_PROFILE_END_SESSION();

	return 0;
}
