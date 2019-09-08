#pragma once

// Client application just has to inherit a class from Application
// and define a create_application() function that returns a derived instance.
// The lib handles the definition of main() here.

extern erwin::Application* erwin::create_application();

int main(int argc, char** argv)
{
	auto app = erwin::create_application();
	app->run();
	delete app;
}
