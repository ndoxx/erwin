// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>

#define W_ENTRY_POINT
#include "erwin.h"
#include "layer_test.h"

using namespace erwin;

/*
	Application for quick and dirty renderer tests
*/
class Editor: public Application
{
public:
	Editor() = default;
	~Editor() = default;

	virtual void on_client_init() override;
	virtual void on_load() override;
	virtual void on_imgui_render() override;
	bool on_keyboard_event(const KeyboardEvent& e);

private:
	LayerTest* layer_;
};

Application* erwin::create_application()
{
	return new Editor();
}
