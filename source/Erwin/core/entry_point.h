#pragma once

extern erwin::Application* erwin::create_application();

int main(int argc, char** argv)
{
	auto app = erwin::create_application();
	app->run();
	delete app;
}