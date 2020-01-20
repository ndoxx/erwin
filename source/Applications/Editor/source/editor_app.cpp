#include "editor_app.h"
#include "debug/logger_thread.h"

void Editor::on_pre_init()
{
	console_ = new ConsoleWidget();
	WLOGGER(create_channel("editor", 3));
	WLOGGER(attach("cw_sink", std::make_unique<editor::ConsoleWidgetSink>(console_), {"editor"_h}));
}

void Editor::on_client_init()
{
	filesystem::set_asset_dir("source/Applications/Sandbox/assets"); // TMP: find a better way to share/centralize assets
	filesystem::set_client_config_dir("source/Applications/Editor/config");
	this->add_configuration("client.xml");
}

void Editor::on_load()
{

	EVENTBUS.subscribe(this, &Editor::on_keyboard_event);

    FramebufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    game_view_fb_ = FramebufferPool::create_framebuffer("game_view"_h, make_scope<FbRatioConstraint>(), layout, false);

    push_layer(game_layer_ = new GameLayer(scene_));
    push_overlay(editor_layer_ = new EditorLayer(scene_));

    editor_layer_->add_widget("console"_h, console_);

    DLOGN("editor") << "Erwin Editor is ready." << std::endl;
}

void Editor::on_unload()
{

}

bool Editor::on_keyboard_event(const KeyboardEvent& e)
{
	// Terminate on ESCAPE
	if(e.pressed && e.key == keymap::WKEY::ESCAPE)
		EVENTBUS.publish(WindowCloseEvent());

	return false;
}

void Editor::on_imgui_render()
{

}
