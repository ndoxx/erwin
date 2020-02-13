#include "editor/layer_editor.h"
#include "editor/font_awesome.h"
#include "editor/editor_components.h"
#include "editor/widget_scene_view.h"
#include "editor/widget_scene_hierarchy.h"
#include "editor/widget_inspector.h"
#include "editor/widget_rt_peek.h"
#include "editor/widget_hex_dump.h"
#include "editor/widget_keybindings.h"
#include "editor/widget_console.h"
#include "editor/scene.h"
#include "debug/logger_thread.h"

using namespace erwin;

namespace editor
{

static struct
{
	bool exit_required = false;
	bool enable_docking = true;

	uint32_t settings_menu;
	uint32_t view_menu;
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

static void set_gui_behavior()
{
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	s_storage.enable_docking = true;
}

EditorLayer::EditorLayer(): Layer("EditorLayer")
{

}

uint32_t EditorLayer::add_menu(const std::string& menu_name)
{
	uint32_t index = menus_.size();
	menus_.push_back({menu_name, {}});
	return index;
}

void EditorLayer::add_widget(uint32_t menu, Widget* widget)
{
	menus_[menu].widgets.push_back(widget);
}

void EditorLayer::on_attach()
{
	set_gui_style();
	set_gui_behavior();

	// Merge icon font
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	auto icon_font_path = filesystem::get_asset_dir() / "fonts" / FONT_ICON_FILE_NAME_FA;
	static ImWchar ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig config;
	config.MergeMode = true;
	io.Fonts->AddFontFromFileTTF(icon_font_path.string().c_str(), 16.0f, &config, ranges);

    // Create ECS component managers
    auto& client_area = Application::get_client_area();
    ECS::create_component_manager<ComponentEditorSelection>(client_area, 2);

	// Load resources
    Scene::init();

	background_shader_ = AssetManager::load_shader("shaders/background.glsl");

    FramebufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    FramebufferPool::create_framebuffer("game_view"_h, make_scope<FbRatioConstraint>(), layout, false);

	// Build UI
	ConsoleWidget* console = new editor::ConsoleWidget();
	WLOGGER(attach("cw_sink", std::make_unique<editor::ConsoleWidgetSink>(console), {"editor"_h, "application"_h, "entity"_h}));

    DLOGN("editor") << "Loading Erwin Editor." << std::endl;
    DLOG("editor",1) << "Creating widgets." << std::endl;

    s_storage.settings_menu = add_menu("Settings");
    add_widget(s_storage.settings_menu, new editor::KeybindingsWidget());

    s_storage.view_menu = add_menu("View");
	add_widget(s_storage.view_menu, console);

    editor::HexDumpWidget* hex_widget;
    add_widget(s_storage.view_menu, hex_widget = new editor::HexDumpWidget());
    hex_widget->refresh();

    add_widget(s_storage.view_menu, new editor::SceneViewWidget());
    add_widget(s_storage.view_menu, new editor::SceneHierarchyWidget());
    add_widget(s_storage.view_menu, new editor::InspectorWidget());

    // Register main render target in peek widget
    editor::RTPeekWidget* peek_widget;
    add_widget(s_storage.view_menu, peek_widget = new editor::RTPeekWidget());
	peek_widget->register_framebuffer("GBuffer");
	peek_widget->register_framebuffer("SpriteBuffer");
	peek_widget->register_framebuffer("BloomCombine");
	peek_widget->register_framebuffer("LBuffer");

    DLOGN("editor") << "Erwin Editor is ready." << std::endl;
}

void EditorLayer::on_detach()
{
	for(auto& desc: menus_)
		for(Widget* widget: desc.widgets)
			delete widget;

	Renderer::destroy(background_shader_);
    Scene::shutdown();    
}

void EditorLayer::on_update(GameClock& clock)
{
	for(auto& desc: menus_)
		for(Widget* widget: desc.widgets)
			widget->on_update();
}

void EditorLayer::on_render()
{
	for(auto& desc: menus_)
		for(Widget* widget: desc.widgets)
			widget->on_layer_render();

	// WTF: we must draw something to the default framebuffer or else, whole screen is blank
	RenderState state;
	state.render_target = Renderer::default_render_target();
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.rasterizer_state.clear_flags = CLEAR_COLOR_FLAG | CLEAR_DEPTH_FLAG;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;

	VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);
	SortKey key;
	key.set_sequence(0, Renderer::next_layer_id(), background_shader_);
	DrawCall dc(DrawCall::Indexed, state.encode(), background_shader_, quad);
	Renderer::submit(key.encode(), dc);
}

void EditorLayer::on_imgui_render()
{
	static bool show_demo_window = false;
	if(ImGui::BeginMainMenuBar())
	{
		if(ImGui::BeginMenu("File"))
		{
        	ImGui::MenuItem("Quit", NULL, &s_storage.exit_required);
        	ImGui::EndMenu();
		}

    	if(ImGui::BeginMenu("Settings"))
    	{
			for(Widget* widget: menus_[s_storage.settings_menu].widgets)
        		ImGui::MenuItem(widget->get_name().c_str(), NULL, &widget->open_);
        	
	    	ImGui::Separator();
	    	ImGui::Checkbox("Docking", &s_storage.enable_docking);
	    	ImGui::Separator();
	    	ImGui::MenuItem("ImGui Demo", NULL, &show_demo_window);

        	ImGui::EndMenu();
    	}

    	if(ImGui::BeginMenu("View"))
    	{
			for(Widget* widget: menus_[s_storage.view_menu].widgets)
        		ImGui::MenuItem(widget->get_name().c_str(), NULL, &widget->open_);
        
        	ImGui::EndMenu();
    	}

    	ImGui::EndMainMenuBar();
    }

    if(s_storage.enable_docking) show_dockspace_window(&s_storage.enable_docking);
    if(show_demo_window) ImGui::ShowDemoWindow();

	for(auto& desc: menus_)
		for(Widget* widget: desc.widgets)
			widget->imgui_render();

	if(s_storage.exit_required)
	{
		EVENTBUS.publish(WindowCloseEvent());
	}
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

} // namespace editor



namespace erwin
{

void Application::build_editor()
{
    DLOG("editor",1) << "Pushing editor layer." << std::endl;
    push_overlay(EDITOR_LAYER = new editor::EditorLayer());
}

} // namespace erwin