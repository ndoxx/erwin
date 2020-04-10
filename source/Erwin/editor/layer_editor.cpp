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
#include "editor/widget_materials.h"
#include "input/input.h"
#include "level/scene.h"
#include "imgui/theme.h"
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

    editor::SceneViewWidget* scene_view_widget_ = nullptr;
} s_storage;

static void set_gui_behavior()
{
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	s_storage.enable_docking = true;
}

EditorLayer::EditorLayer():
Layer("EditorLayer")
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

    // Reflect editor components
    REFLECT_COMPONENT(ComponentEditorDescription);

	// Load resources
    Scene::init();

	background_shader_ = AssetManager::load_shader("shaders/background.glsl");

    FramebufferLayout layout
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

    s_storage.scene_view_widget_ = new editor::SceneViewWidget();
    add_widget(s_storage.view_menu, s_storage.scene_view_widget_);
    add_widget(s_storage.view_menu, new editor::SceneHierarchyWidget());
    add_widget(s_storage.view_menu, new editor::MaterialsWidget());
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

bool EditorLayer::on_event(const MouseButtonEvent& event)
{
    // If scene view is not hovered, let ImGui consume input events
    if(!s_storage.scene_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
            return true;
    }

    return s_storage.scene_view_widget_->on_mouse_event(event);
}

bool EditorLayer::on_event(const WindowResizeEvent& event)
{
	return false;
}

bool EditorLayer::on_event(const MouseScrollEvent& event)
{
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

bool EditorLayer::on_event(const KeyboardEvent& event)
{
    // If scene view is not hovered, let ImGui consume input events
    if(!s_storage.scene_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureKeyboard)
            return true;
    }

    if(event.pressed && !event.repeat && event.key == Input::get_action_key(ACTION_DROP_SELECTION))
    {
        Scene::drop_selection();
        return true;
    }

    return false;
}

bool EditorLayer::on_event(const KeyTypedEvent& event)
{
    // Don't propagate event if ImGui wants to handle it
    ImGuiIO& io = ImGui::GetIO();
    return io.WantTextInput;
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