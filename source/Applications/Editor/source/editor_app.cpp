#include "editor_app.h"
#include "widget_game_view.h"
#include "debug/logger_thread.h"

static struct
{
	bool exit_required = false;
} s_storage;

static void set_gui_style()
{
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();
	if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	else
	{
    	style.WindowRounding = 5.0f;
	}

    ImVec4 neutral(0.05f, 0.3f, 0.7f, 1.0f);
    ImVec4 active(0.05f, 0.7f, 0.5f, 1.0f);
    ImVec4 hovered(0.6f, 0.6f, 0.6f, 1.0f);
    ImVec4 inactive(0.05f, 0.7f, 0.3f, 0.75f);

    style.Colors[ImGuiCol_Border] = ImVec4(0.3f, 0.3f, 0.3f, 1.f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    style.Colors[ImGuiCol_TitleBg] = neutral;
    style.Colors[ImGuiCol_TitleBgCollapsed] = inactive;
    style.Colors[ImGuiCol_TitleBgActive] = active;

    style.Colors[ImGuiCol_Header] = neutral;
    style.Colors[ImGuiCol_HeaderHovered] = hovered;
    style.Colors[ImGuiCol_HeaderActive] = active;

    style.Colors[ImGuiCol_ResizeGrip] = neutral;
    style.Colors[ImGuiCol_ResizeGripHovered] = hovered;
    style.Colors[ImGuiCol_ResizeGripActive] = active;

    style.Colors[ImGuiCol_Button] = neutral;
    style.Colors[ImGuiCol_ButtonHovered] = hovered;
    style.Colors[ImGuiCol_ButtonActive] = active;

    style.Colors[ImGuiCol_SliderGrab] = hovered;
    style.Colors[ImGuiCol_SliderGrabActive] = active;

    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.1f, 0.8f, 0.2f, 1.0f);
}

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
	set_gui_style();

	EVENTBUS.subscribe(this, &Editor::on_keyboard_event);

    FramebufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    game_view_fb_ = FramebufferPool::create_framebuffer("game_view"_h, make_scope<FbRatioConstraint>(), layout, false);

    push_layer(layer_ = new LayerTest(scene_));

    widgets_.insert(std::make_pair("console"_h, console_));
    widgets_.insert(std::make_pair("game"_h, new GameViewWidget()));

    DLOGN("editor") << "Erwin Editor is ready." << std::endl;
}

void Editor::on_unload()
{
	for(auto&& [key,widget]: widgets_)
		delete widget;
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
	static bool show_dockspace = false;
	static bool show_demo_window = false;
	if(ImGui::BeginMainMenuBar())
	{
		if(ImGui::BeginMenu("File"))
		{
        	ImGui::MenuItem("Quit", NULL, &s_storage.exit_required);
        	ImGui::EndMenu();
		}
    	if(ImGui::BeginMenu("Windows"))
    	{
        	ImGui::MenuItem("Console", NULL, &widgets_["console"_h]->open_);
        	ImGui::MenuItem("Game",    NULL, &widgets_["game"_h]->open_);
        	ImGui::Separator();
        	ImGui::MenuItem("Docking", NULL, &show_dockspace);
        	ImGui::MenuItem("ImGui Demo", NULL, &show_demo_window);
        	ImGui::EndMenu();
    	}
    	ImGui::EndMainMenuBar();
    }

    if(show_dockspace)   show_dockspace_window(&show_dockspace);
    if(show_demo_window) ImGui::ShowDemoWindow();

	for(auto&& [key,widget]: widgets_)
		widget->render();

	if(s_storage.exit_required)
	{
		EVENTBUS.publish(WindowCloseEvent());
	}
}

void Editor::show_dockspace_window(bool* p_open)
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
    ImGui::Begin("DockSpace Demo", p_open, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();
}
