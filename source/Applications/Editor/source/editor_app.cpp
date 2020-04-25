#include "editor_app.h"
#include "layer_game.h"
#include "editor/layer_editor.h"
#include "editor/layer_editor_background.h"

void ErwinEditor::on_pre_init()
{

}

void ErwinEditor::on_client_init()
{
	filesystem::set_asset_dir("source/Applications/Editor/assets");
	filesystem::set_client_config_dir("source/Applications/Editor/config");
	add_configuration("client.xml");
}

void ErwinEditor::on_load()
{
	EVENTBUS.subscribe(this, &ErwinEditor::on_keyboard_event);

    DLOG("application",1) << "Pushing game layer." << std::endl;
    push_overlay(editor_layer_ = new editor::EditorLayer());
    push_overlay(editor_background_layer_ = new editor::EditorBackgroundLayer());
    push_layer(game_layer_ = new GameLayer());

    // If editor is enabled, PPRenderer should draw to the host window framebuffer instead of the default one
    PostProcessingRenderer::set_final_render_target("host"_h);
}

void ErwinEditor::on_unload()
{

}

bool ErwinEditor::on_keyboard_event(const KeyboardEvent& e)
{
	// Terminate on Ctrl+ESCAPE
	if(e.pressed && e.key == keymap::WKEY::ESCAPE && (e.mods & keymap::WKEYMOD::CONTROL))
		EVENTBUS.publish(WindowCloseEvent());

	return false;
}

void ErwinEditor::on_imgui_render()
{

}
