#include "editor_app.h"
#include "entity/component/editor_tags.h"
#include "entity/component/tags.h"
#include "imgui/font_awesome.h"
#include "imgui/theme.h"
#include "layer/layer_material_editor.h"
#include "layer/layer_post_processing.h"
#include "layer/layer_scene_editor.h"
#include "layer/layer_scene_view.h"
#include "level/scene.h"
#include "level/scene_manager.h"
#include "project/project.h"
#include "widget/dialog_open.h"
#include "widget/widget_console.h"
#include "widget/widget_keybindings.h"
#include <kibble/logger/dispatcher.h>

#include <fstream>

static void set_gui_behavior()
{
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
}

ErwinEditor::ErwinEditor() : Application({"erwin", "editor"}) {}

void ErwinEditor::on_client_init()
{
    auto root = WFS_.get_aliased_directory("root"_h);
    WFS_.alias_directory(root / "source/Applications/Editor/assets", "res");
    WFS_.alias_directory(root / "source/Applications/Editor/config", "cfg");
    add_configuration("cfg://client.toml");
    add_configuration("usr://settings.toml", "cfg://default_settings.toml");
}

void ErwinEditor::on_load()
{
    // * Create host framebuffer in order to render to an ImGui window
    FramebufferLayout layout{{"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}};
    FramebufferPool::create_framebuffer("host"_h, make_scope<FbRatioConstraint>(), FB_NONE, layout);

    // * Configure GUI
    theme::init();
    theme::load_default();
    set_gui_behavior();

    // Merge icon font
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    auto icon_font_path = WFS_.regular_path("res://fonts") / FONT_ICON_FILE_NAME_FA;
    static ImWchar ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig config;
    config.MergeMode = true;
    io.Fonts->AddFontFromFileTTF(icon_font_path.c_str(), 16.0f, &config, ranges);

    console_ = new editor::ConsoleWidget(get_event_bus());
    KLOGGER(attach("cw_sink", std::make_unique<editor::ConsoleWidgetSink>(console_),
                   {"editor"_h, "application"_h, "entity"_h, "scene"_h, "script"_h}));
    keybindings_widget_ = new editor::KeybindingsWidget(get_event_bus());

    KLOGN("editor") << "Loading Erwin Editor." << std::endl;

    scene_view_layer_ = new editor::SceneViewLayer(*this);
    scene_editor_layer_ = new editor::SceneEditorLayer(*this);
    auto* material_editor_layer = new editor::MaterialEditorLayer(*this);
    push_layer(scene_view_layer_);
    push_overlay(scene_editor_layer_);
    push_overlay(material_editor_layer, false);
    push_overlay(new editor::PostProcessingLayer(*this));

    get_event_bus().subscribe(this, &ErwinEditor::on_keyboard_event);

    create_state(EditorStateIdx::SCENE_EDITION, {"Scene edition", {scene_view_layer_}, scene_editor_layer_});
    create_state(EditorStateIdx::MATERIAL_AUTHORING, {"Material authoring", {}, material_editor_layer});

    SceneManager::create_scene("main_scene"_h);
    SceneManager::make_current("main_scene"_h);

    // Setup scene injection
    scn::current().set_injector_callback([this](Scene& scene) {
        auto root = scene.get_named("root"_h);
        scene.add_component<FixedHierarchyTag>(root);
        scene.add_component<NonEditableTag>(root);
        scene.add_component<NonRemovableTag>(root);
        scene.add_component<NoGizmoTag>(root);

        scene_editor_layer_->setup_editor_entities(scene);
    });

    // Setup scene finisher callback
    scn::current().set_finisher_callback([this](Scene& scene) {
        scene_view_layer_->setup_camera(scene);
        auto e_cam = scene.get_named("Camera"_h);
        scene.add_component<FixedHierarchyTag>(e_cam);
        scene.add_component<NonRemovableTag>(e_cam);
        scene.add_component<NoGizmoTag>(e_cam);

        /*
        #ifdef W_DEBUG
                ScriptEngine::get_context(scene.get_script_context()).dbg_dump_state("out.log");
        #endif
        */
    });

    // Project settings
    bool auto_load = CFG_.get<bool>("settings.project.auto_load"_h, true);
    auto last_project_file = CFG_.get<std::string>("settings.project.last_project"_h, "");
    if(auto_load && !last_project_file.empty() && WFS_.exists(last_project_file))
    {
        KLOGN("editor") << "Opening last project:" << std::endl;
        KLOGI << KS_PATH_ << last_project_file << std::endl;
        project::load_project(last_project_file);
        const auto& ps = project::get_project_settings();
        auto start_scene = ps.registry.get<std::string>("project.scene.start"_h, "");
        if(!start_scene.empty())
            scn::current().load_xml(start_scene);
    }

    KLOGN("editor") << "Erwin Editor is ready." << std::endl;

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
        get_event_bus().enqueue(WindowCloseEvent());
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
            if(ImGui::MenuItem("Load project", nullptr, nullptr))
                dialog::show_open("ChooseFileDlgKey", "Choose Project File", ".erwin", ".");

            if(ImGui::MenuItem("Save project", nullptr, nullptr))
            {
                project::save_project();
                auto& scene = scn::current();
                if(!scene.get_file_location().empty() && WFS_.exists(scene.get_file_location()))
                    scene.save();
                else
                    dialog::show_open("ScnSaveAsDlgKey", "Save scene as", ".scn",
                                      project::asset_dir(DK::SCENE));
            }

            if(ImGui::MenuItem("Close project", nullptr, nullptr))
            {
                project::close_project();
                SceneManager::unload_scene("main_scene"_h);
            }

            ImGui::Separator();
            ImGui::MenuItem("Quit", nullptr, &exit_required_);
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Settings"))
        {
            if(ImGui::BeginMenu("Render"))
            {
                if(ImGui::MenuItem("VSync", nullptr, &vsync_enabled_))
                    enable_vsync(vsync_enabled_);
                ImGui::EndMenu();
            }

            ImGui::Separator();
            ImGui::MenuItem(keybindings_widget_->get_name().c_str(), nullptr, &keybindings_widget_->open_);

            ImGui::Separator();
            const auto& themes = theme::get_list();
            if(ImGui::BeginMenu("Theme"))
            {
                for(const auto& entry : themes)
                {
                    if(ImGui::MenuItem(entry.name.c_str()))
                        theme::load(entry);
                }
                ImGui::Separator();
                if(ImGui::MenuItem("ImGui::Classic"))
                {
                    theme::reset();
                    ImGui::StyleColorsClassic();
                }
                if(ImGui::MenuItem("ImGui::Light"))
                {
                    theme::reset();
                    ImGui::StyleColorsLight();
                }
                if(ImGui::MenuItem("ImGui::Dark"))
                {
                    theme::reset();
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
                if(ImGui::MenuItem(state.name.c_str(), nullptr, checked))
                    switch_state(idx);
            }
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem(console_->get_name().c_str(), nullptr, &console_->open_);
            for(Widget* widget : states_[size_t(current_state_idx_)].gui_layer->get_widgets())
                ImGui::MenuItem(widget->get_name().c_str(), nullptr, &widget->open_);

            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &s_show_demo_window);

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Dialogs
    dialog::on_open("ChooseFileDlgKey", [](const fs::path& filepath) {
        project::load_project(std::string(filepath));
        SceneManager::make_current("main_scene"_h);
        const auto& ps = project::get_project_settings();
        auto scene_path = ps.registry.get<std::string>("project.scene.start"_h, "");
        if(!scene_path.empty())
            scn::current().load_xml(scene_path);
    });

    dialog::on_open("ScnSaveAsDlgKey", [](const fs::path& filepath) {
        auto& scene = scn::current();
        scene.save_xml(std::string(filepath));
    });

    if(enable_docking_)
        show_dockspace_window(&enable_docking_);
    if(s_show_demo_window)
        ImGui::ShowDemoWindow();

    console_->imgui_render();
    keybindings_widget_->imgui_render();

    if(exit_required_)
    {
        get_event_bus().enqueue(WindowCloseEvent());
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
    K_ASSERT(idx < EditorStateIdx::COUNT, "State index out of bounds");
    states_[size_t(idx)] = state;
}

void ErwinEditor::switch_state(EditorStateIdx idx)
{
    if(idx == current_state_idx_)
        return;

    K_ASSERT(idx < EditorStateIdx::COUNT, "State index out of bounds");
    states_[size_t(current_state_idx_)].enable(false);
    states_[size_t(idx)].enable(true);

    current_state_idx_ = idx;
}

EditorStateIdx ErwinEditor::cycle_state()
{
    size_t next = size_t(current_state_idx_);
    next = (next + 1) % size_t(EditorStateIdx::COUNT);
    switch_state(EditorStateIdx(next));
    return current_state_idx_;
}
