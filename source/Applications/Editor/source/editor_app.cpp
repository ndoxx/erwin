#include "editor_app.h"
#include "debug/logger_thread.h"

static struct
{
	bool exit_required = false;
} s_storage;

void Editor::on_pre_init()
{
	console_ = new ConsoleWidget();
	WLOGGER(create_channel("editor", 3));
	WLOGGER(attach("cw_sink", std::make_unique<editor::ConsoleWidgetSink>(console_), {"editor"_h}));
}

void Editor::on_client_init()
{
	filesystem::set_asset_dir("source/Applications/Editor/assets");
	filesystem::set_client_config_dir("source/Applications/Editor/config");
	this->add_configuration("client.xml");
}

void Editor::on_load()
{
	EVENTBUS.subscribe(this, &Editor::on_keyboard_event);

    push_layer(layer_ = new LayerTest());

    widgets_.insert(std::make_pair("console"_h, console_));

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
