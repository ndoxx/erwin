#include "test_application.h"
#include <kibble/logger/dispatcher.h>

// ENTRY POINT
extern GfxTestApplication* create_application();

int main(int /*argc*/, char** /*argv*/)
{
    KLOGGER_START();
	
	auto app = create_application();
	if(!app->init())
	{
		delete app;
		return -1;
	}

	app->run();
	app->shutdown();
	delete app;

	return 0;
}