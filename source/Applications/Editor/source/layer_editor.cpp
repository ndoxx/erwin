#include "layer_editor.h"
#include "widget_game_view.h"
#include "erwin.h"

using namespace erwin;
using namespace editor;

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
    ImVec4 active(1.f, 0.5f, 0.05f, 1.0f);
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

EditorLayer::EditorLayer(editor::Scene& scene): Layer("EditorLayer"), scene_(scene)
{

}

void EditorLayer::on_imgui_render()
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

    // game_layer_->set_enabled(widgets_["game"_h]->open_);

    if(show_dockspace)   show_dockspace_window(&show_dockspace);
    if(show_demo_window) ImGui::ShowDemoWindow();

	for(auto&& [key,widget]: widgets_)
		widget->render();

	if(s_storage.exit_required)
	{
		EVENTBUS.publish(WindowCloseEvent());
	}
}

void EditorLayer::on_attach()
{
	set_gui_style();

    add_widget("game"_h, new GameViewWidget());
}

void EditorLayer::on_detach()
{
	for(auto&& [key,widget]: widgets_)
		delete widget;
}

void EditorLayer::on_update(GameClock& clock)
{

}

void EditorLayer::on_render()
{

}

bool EditorLayer::on_event(const MouseButtonEvent& event)
{
	return false;
}

bool EditorLayer::on_event(const WindowResizeEvent& event)
{
	return false;
}

bool EditorLayer::on_event(const MouseScrollEvent& event)
{
	return false;
}

bool EditorLayer::on_event(const KeyboardEvent& event)
{
	if(event.pressed && event.key == keymap::WKEY::F1)
		scene_.camera_controller.toggle_control();

	return false;
}


void EditorLayer::show_dockspace_window(bool* p_open)
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