// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>
#include <map>

#define W_ENTRY_POINT
#include "erwin.h"
#include "layer_test.h"
#include "widget.h"
#include "widget_console.h"

using namespace erwin;
using namespace editor;

/*
	Level and asset editor for Erwin Engine
*/
class Editor: public Application
{
public:
	Editor() = default;
	~Editor() = default;

	virtual void on_pre_init() override;
	virtual void on_client_init() override;
	virtual void on_load() override;
	virtual void on_unload() override;
	virtual void on_imgui_render() override;
	bool on_keyboard_event(const KeyboardEvent& e);

	void show_dockspace_window(bool* p_open);

private:
	LayerTest* layer_;

	ConsoleWidget* console_;
	std::map<hash_t, Widget*> widgets_;
};

Application* erwin::create_application()
{
	return new Editor();
}
