#include "editor_app.h"
#include "debug/logger_thread.h"
#include "imgui/font_awesome.h"
#include "imgui/theme.h"
#include "layer/layer_scene_view.h"
#include "layer/layer_scene_editor.h"
#include "layer/layer_material_editor.h"
#include "layer/layer_editor_background.h"
#include "widget/widget_console.h"
#include "widget/widget_keybindings.h"
#include "project/project.h"

static void set_gui_behavior()
{
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
}

void ErwinEditor::on_pre_init() {}

void ErwinEditor::on_client_init()
{
    filesystem::set_asset_dir("source/Applications/Editor/assets");
    filesystem::set_client_config_dir("source/Applications/Editor/config");
    add_configuration("client.xml");
}

void ErwinEditor::on_load()
{
    // * Create host framebuffer in order to render to an ImGui window
    FramebufferLayout layout{{"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}};
    FramebufferPool::create_framebuffer("host"_h, make_scope<FbRatioConstraint>(), FB_NONE, layout);

    // * Configure GUI
    editor::theme::init();
    editor::theme::load_default();
    set_gui_behavior();

    // Merge icon font
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    auto icon_font_path = filesystem::get_asset_dir() / "fonts" / FONT_ICON_FILE_NAME_FA;
    static ImWchar ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig config;
    config.MergeMode = true;
    io.Fonts->AddFontFromFileTTF(icon_font_path.string().c_str(), 16.0f, &config, ranges);

    console_ = new editor::ConsoleWidget();
    WLOGGER(attach("cw_sink", std::make_unique<editor::ConsoleWidgetSink>(console_),
                   {"editor"_h, "application"_h, "entity"_h}));
    keybindings_widget_ = new editor::KeybindingsWidget();

    DLOGN("editor") << "Loading Erwin Editor." << std::endl;

    auto* scene_view_layer = new SceneViewLayer();
    auto* scene_editor_layer = new editor::SceneEditorLayer();
    auto* material_editor_layer = new editor::MaterialEditorLayer();
    push_layer(scene_view_layer);
    push_overlay(scene_editor_layer);
    push_overlay(material_editor_layer, false);
    push_overlay(new editor::EditorBackgroundLayer());

    EventBus::subscribe(this, &ErwinEditor::on_keyboard_event);

    create_state(EditorStateIdx::SCENE_EDITION,      {"Scene edition", {scene_view_layer}, scene_editor_layer});
    create_state(EditorStateIdx::MATERIAL_AUTHORING, {"Material authoring", {}, material_editor_layer});

    // Project settings
    project::load_global_settings();

    DLOGN("editor") << "Erwin Editor is ready." << std::endl;

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
    if(Input::match_action(ACTION_EDITOR_QUIT, e))
    {
        EventBus::enqueue(WindowCloseEvent());
        return true;
    }

    // Cycle editor state
    if(Input::match_action(ACTION_EDITOR_CYCLE_MODE, e))
    {
    	cycle_state();
    	return true;
    }

    return false;
}

static bool s_show_demo_window = false;
void ErwinEditor::on_imgui_render()
{
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("Quit", NULL, &exit_required_);
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Settings"))
        {
            if(ImGui::BeginMenu("Render"))
            {
                if(ImGui::MenuItem("VSync", NULL, &vsync_enabled_))
                    enable_vsync(vsync_enabled_);
                ImGui::EndMenu();
            }

            ImGui::Separator();
            ImGui::MenuItem(keybindings_widget_->get_name().c_str(), NULL, &keybindings_widget_->open_);

            ImGui::Separator();
            const auto& themes = editor::theme::get_list();
            if(ImGui::BeginMenu("Theme"))
            {
                for(const auto& entry : themes)
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

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Mode"))
        {
            for(size_t ii = 0; ii < size_t(EditorStateIdx::COUNT); ++ii)
            {
                EditorStateIdx idx = EditorStateIdx(ii);
                const auto& state = states_[ii];
                bool checked = (current_state_idx_ == idx);
                if(ImGui::MenuItem(state.name.c_str(), NULL, checked))
                    switch_state(idx);
            }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem(console_->get_name().c_str(), NULL, &console_->open_);
            for(Widget* widget : states_[size_t(current_state_idx_)].gui_layer->get_widgets())
                ImGui::MenuItem(widget->get_name().c_str(), NULL, &widget->open_);

            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", NULL, &s_show_demo_window);

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if(enable_docking_)
        show_dockspace_window(&enable_docking_);
    if(s_show_demo_window)
        ImGui::ShowDemoWindow();

    console_->imgui_render();
    keybindings_widget_->imgui_render();

    if(exit_required_)
    {
        EventBus::enqueue(WindowCloseEvent());
    }
}

static ImGuiDockNodeFlags s_dockspace_flags = ImGuiDockNodeFlags_None;
void ErwinEditor::show_dockspace_window(bool* p_open)
{
    bool opt_fullscreen = true;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if(opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if(s_dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", p_open, window_flags);
    ImGui::PopStyleVar();

    if(opt_fullscreen)
        ImGui::PopStyleVar(2);

    ImGuiIO& io = ImGui::GetIO();
    if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), s_dockspace_flags);
    }

    ImGui::End();
}

void ErwinEditor::create_state(EditorStateIdx idx, EditorState&& state)
{
    W_ASSERT(idx < EditorStateIdx::COUNT, "State index out of bounds");
    states_[size_t(idx)] = state;
}

void ErwinEditor::switch_state(EditorStateIdx idx)
{
    if(idx == current_state_idx_)
        return;

    W_ASSERT(idx < EditorStateIdx::COUNT, "State index out of bounds");
    states_[size_t(current_state_idx_)].enable(false);
    states_[size_t(idx)].enable(true);

    current_state_idx_ = idx;
}

EditorStateIdx ErwinEditor::cycle_state()
{
	size_t next = size_t(current_state_idx_);
	next = (next+1)%size_t(EditorStateIdx::COUNT);
	switch_state(EditorStateIdx(next));
	return current_state_idx_;
}
