#include "editor_app.h"
#include "layer_game.h"
#include "editor/font_awesome.h"
#include "editor/layer_editor_scene.h"
#include "editor/layer_editor_background.h"
#include "editor/widget_console.h"
#include "editor/widget_keybindings.h"
#include "imgui/theme.h"
#include "debug/logger_thread.h"

static void set_gui_behavior()
{
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;
}

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

	// * Create host framebuffer in order to render to an ImGui window
    FramebufferLayout layout
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    FramebufferPool::create_framebuffer("host"_h, make_scope<FbRatioConstraint>(), FB_NONE, layout);

    // * Configure GUI
    editor::theme::init();
    editor::theme::load_default();
	set_gui_behavior();

	// Merge icon font
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	auto icon_font_path = filesystem::get_asset_dir() / "fonts" / FONT_ICON_FILE_NAME_FA;
	static ImWchar ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig config;
	config.MergeMode = true;
	io.Fonts->AddFontFromFileTTF(icon_font_path.string().c_str(), 16.0f, &config, ranges);


	console_ = new editor::ConsoleWidget();
	WLOGGER(attach("cw_sink", std::make_unique<editor::ConsoleWidgetSink>(console_), {"editor"_h, "application"_h, "entity"_h}));

	keybindings_widget_ = new editor::KeybindingsWidget();

    DLOGN("editor") << "Loading Erwin Editor." << std::endl;
    push_overlay(scene_editor_layer_ = new editor::SceneEditorLayer());
    push_overlay(editor_background_layer_ = new editor::EditorBackgroundLayer());
    DLOG("application",1) << "Pushing game layer." << std::endl;
    push_layer(game_layer_ = new GameLayer());

    // If editor is enabled, PPRenderer should draw to the host window framebuffer instead of the default one
    PostProcessingRenderer::set_final_render_target("host"_h);
}

void ErwinEditor::on_unload()
{
	delete keybindings_widget_;
	delete console_;
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
	static bool show_demo_window = false;
	if(ImGui::BeginMainMenuBar())
	{
		if(ImGui::BeginMenu("File"))
		{
        	ImGui::MenuItem("Quit", NULL, &exit_required_);
        	ImGui::EndMenu();
		}

    	if(ImGui::BeginMenu("Settings"))
    	{
    		ImGui::MenuItem(keybindings_widget_->get_name().c_str(), NULL, &keybindings_widget_->open_);
        	
            ImGui::Separator();
            const auto& themes = editor::theme::get_list();
            if(ImGui::BeginMenu("Theme"))
            {
                for(const auto& entry: themes)
                {
                    if(ImGui::MenuItem(entry.name.c_str()))
                        editor::theme::load(entry);
                }
                ImGui::Separator();
                if(ImGui::MenuItem("ImGui::Classic"))
                {
                    editor::theme::reset();
                    ImGui::StyleColorsClassic();
                }
                if(ImGui::MenuItem("ImGui::Light"))
                {
                    editor::theme::reset();
                    ImGui::StyleColorsLight();
                }
                if(ImGui::MenuItem("ImGui::Dark"))
                {
                    editor::theme::reset();
                    ImGui::StyleColorsDark();
                }

                ImGui::EndMenu();
            }

	    	ImGui::Separator();
	    	ImGui::Checkbox("Docking", &enable_docking_);
	    	ImGui::Separator();
	    	ImGui::MenuItem("ImGui Demo", NULL, &show_demo_window);

        	ImGui::EndMenu();
    	}

    	if(ImGui::BeginMenu("View"))
    	{
			for(Widget* widget: scene_editor_layer_->get_widgets())
        		ImGui::MenuItem(widget->get_name().c_str(), NULL, &widget->open_);
        
        	ImGui::EndMenu();
    	}
    }


    if(enable_docking_) show_dockspace_window(&enable_docking_);
    if(show_demo_window) ImGui::ShowDemoWindow();

	console_->imgui_render();

	if(exit_required_)
	{
		EVENTBUS.publish(WindowCloseEvent());
	}
}


void ErwinEditor::show_dockspace_window(bool* p_open)
{
    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", p_open, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();
}