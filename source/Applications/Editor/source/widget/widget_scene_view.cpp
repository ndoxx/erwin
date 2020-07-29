#include "widget/widget_scene_view.h"
#include "asset/bounding.h"
#include "asset/material.h"
#include "core/application.h"
#include "core/config.h"
#include "entity/component/bounding_box.h"
#include "entity/component/transform.h"
#include "entity/reflection.h"
#include "event/event_bus.h"
#include "event/window_events.h"
#include "imgui.h"
#include "imgui/color.h"
#include "imgui/font_awesome.h"
#include "input/input.h"
#include "level/scene_manager.h"
#include "project/project.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"
#include "script/script_engine.h"
#include "widget/dialog_open.h"
#include "widget/overlay_gizmo.h"
#include "widget/overlay_stats.h"

using namespace erwin;

namespace editor
{

static constexpr float k_border = 4.f;
static constexpr float k_start_x = 4.f;
static constexpr float k_start_y = 43.f;
static constexpr float k_overlay_dist = 10.f;

SceneViewWidget::SceneViewWidget() : Widget("Scene", true), render_surface_{0.f, 0.f, 0.f, 0.f, 0.f, 0.f}
{
    flags_ |= ImGuiWindowFlags_MenuBar;
    enable_runtime_profiling_ = cfg::get<bool>("erwin.profiling.runtime_session_enabled"_h, false);
    track_next_frame_draw_calls_ = false;
    runtime_ = false;
    paused_ = false;
    stats_overlay_ = new RenderStatsOverlay();
    gizmo_overlay_ = new GizmoOverlay();
}

SceneViewWidget::~SceneViewWidget()
{
    delete gizmo_overlay_;
    delete stats_overlay_;
}

void SceneViewWidget::setup_editor_entities(erwin::Scene& scene) { gizmo_overlay_->setup_editor_entities(scene); }

void SceneViewWidget::on_update(const GameClock& clock)
{
    stats_overlay_->on_update(clock);
    gizmo_overlay_->on_update(clock);

    if(track_next_frame_draw_calls_)
    {
        Renderer::track_draw_calls("draw_calls.json");
        track_next_frame_draw_calls_ = false;
    }
}

void SceneViewWidget::on_resize(uint32_t width, uint32_t height)
{
    float rw = std::max(float(width) - (k_border + k_start_x), 1.f);
    float rh = std::max(float(height) - (k_border + k_start_y), 1.f);
    render_surface_.x1 = render_surface_.x0 + rw;
    render_surface_.y1 = render_surface_.y0 + rh;
    render_surface_.w = rw;
    render_surface_.h = rh;

    EventBus::enqueue(WindowResizeEvent(int(width), int(height)));
    EventBus::enqueue(FramebufferResizeEvent(int(rw), int(rh)));
}

void SceneViewWidget::on_move(int32_t x, int32_t y)
{
    float rw = std::max(float(width_) - (k_border + k_start_x), 1.f);
    float rh = std::max(float(height_) - (k_border + k_start_y), 1.f);
    render_surface_.x0 = float(x) + k_start_x;
    render_surface_.y0 = float(y) + k_start_y;
    render_surface_.x1 = render_surface_.x0 + rw;
    render_surface_.y1 = render_surface_.y0 + rh;

    EventBus::enqueue(WindowMovedEvent(x, y));
}

bool SceneViewWidget::on_mouse_event(const erwin::MouseButtonEvent& event)
{
    // If gizmo is hovered, consume event
    if(gizmo_overlay_->is_hovered())
        return true;

    if(event.button == keymap::WMOUSE::BUTTON_0 && event.pressed && event.x > render_surface_.x0 &&
       event.x < render_surface_.x1 && event.y > render_surface_.y0 && event.y < render_surface_.y1)
    {
        glm::vec2 coords = {(event.x - render_surface_.x0) / render_surface_.w,
                            1.f - (event.y - render_surface_.y0) / render_surface_.h};

        EventBus::enqueue(RaySceneQueryEvent(coords));

        return true;
    }

    return false;
}

bool SceneViewWidget::on_keyboard_event(const erwin::KeyboardEvent& event)
{
    if(Input::match_action(ACTION_EDITOR_CYCLE_GIZMO, event) && !gizmo_overlay_->is_in_use())
    {
        gizmo_overlay_->cycle_operation();
        return true;
    }

    return false;
}

static bool s_show_frame_profiler = false;
static bool s_enable_debug_show_uv = false;
void SceneViewWidget::on_imgui_render()
{
    auto& scene = scn::current();
    if(ImGui::BeginMenuBar())
    {
        if(project::is_loaded() && !scene.is_runtime() && ImGui::BeginMenu("File"))
        {
            if(ImGui::BeginMenu("Load"))
            {
                // List all available scenes for current project
                auto scene_dir = project::asset_dir(DK::SCENE);
                for(auto& entry : fs::directory_iterator(scene_dir.absolute()))
                {
                    if(entry.is_regular_file() && !entry.path().extension().string().compare(".scn"))
                    {
                        if(ImGui::MenuItem(entry.path().filename().c_str()))
                        {
                            // TODO: check that we are not in runtime mode
                            scene.unload();
                            scene.load_xml(WPath(entry.path()));
                        }
                    }
                }

                ImGui::EndMenu();
            }

            bool save_as_needed = false;
            if(ImGui::MenuItem("Save"))
            {
                if(!scene.get_file_location().empty() && scene.get_file_location().exists())
                    scene.save();
                else
                    save_as_needed = true;
            }

            if(ImGui::MenuItem("Save as") || save_as_needed)
            {
                dialog::show_open("ScnSaveAsDlgKey", "Save scene as", ".scn", project::asset_dir(DK::SCENE).absolute());
            }

            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Profiling"))
        {
            ImGui::MenuItem("Frame profiler", NULL, &s_show_frame_profiler);
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Overlays"))
        {
            ImGui::MenuItem("Render stats", NULL, &stats_overlay_->open_);
            ImGui::MenuItem("Manipulator", NULL, &gizmo_overlay_->open_);
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Debug"))
        {
            if(ImGui::MenuItem("Show UV", NULL, &s_enable_debug_show_uv))
                Renderer3D::debug_show_uv(s_enable_debug_show_uv);
            ImGui::EndMenu();
        }
        ImGui::Separator();

        // * Toolbar
        // Scene runtime state indicator
        if(!runtime_)
            ImGui::TextColored({0.1f, 1.0f, 0.1f, 1.f}, "[EDITING]");
        else if(!paused_)
            ImGui::TextColored({1.0f, 0.1f, 0.1f, 1.f}, "[RUNTIME]");
        else
            ImGui::TextColored({1.0f, 0.7f, 0.1f, 1.f}, "[ PAUSE ]");

        // Scene runtime state modifiers
        if(!runtime_ && ImGui::Button(W_ICON(PLAY)))
            runtime_start();
        else if(runtime_ && ImGui::Button(W_ICON(STOP)))
            runtime_stop();

        if(runtime_)
        {
            if(ImGui::Button(W_ICON(PAUSE)))
                runtime_pause();
            if(paused_ && ImGui::Button(W_ICON(STEP_FORWARD)))
                Application::get_instance().get_clock().require_next_frame();
            if(ImGui::Button(W_ICON(REPEAT)))
                runtime_reset();

            if(!paused_)
            {
                ImGui::Separator();
                if(ImGui::Button(W_ICON(CARET_UP)))
                    Application::get_instance().get_clock().frame_speed_up();
                if(ImGui::Button(W_ICON(CARET_DOWN)))
                    Application::get_instance().get_clock().frame_slow_down();
                if(ImGui::Button(W_ICON(SORT)))
                    Application::get_instance().get_clock().set_frame_speed(1.f);
            }
        }

        ImGui::EndMenuBar();
    }

    if(s_show_frame_profiler)
        frame_profiler_window(&s_show_frame_profiler);

    if(stats_overlay_->open_)
    {
        ImVec2 overlay_pos(render_surface_.x1 - k_overlay_dist, render_surface_.y0 + k_overlay_dist);
        ImVec2 overlay_pivot(1.f, 0.f);
        ImGui::SetNextWindowPos(overlay_pos, ImGuiCond_Always, overlay_pivot);
        ImGui::SetNextWindowBgAlpha(0.35f);
        stats_overlay_->imgui_render();
    }

    if(gizmo_overlay_->open_)
    {
        ImVec2 overlay_pos(render_surface_.x1 - k_overlay_dist, render_surface_.y1 - k_overlay_dist);
        ImVec2 overlay_pivot(1.f, 1.f);
        ImGui::SetNextWindowPos(overlay_pos, ImGuiCond_Always, overlay_pivot);
        ImGui::SetNextWindowBgAlpha(0.95f);
        gizmo_overlay_->imgui_render();
    }

    dialog::on_open("ScnSaveAsDlgKey", [&scene](const fs::path& filepath) { scene.save_xml(WPath(filepath)); });

    // * Show game render in window
    // Retrieve the native framebuffer texture handle
    FramebufferHandle fb = FramebufferPool::get_framebuffer("host"_h);
    TextureHandle texture = Renderer::get_framebuffer_texture(fb, 0);
    void* framebuffer_texture_native = Renderer::get_native_texture_handle(texture);
    ImGui::GetWindowDrawList()->AddImage(framebuffer_texture_native,
                                         // ImGui::GetCursorScreenPos(),
                                         ImVec2(render_surface_.x0, render_surface_.y0),
                                         ImVec2(render_surface_.x1, render_surface_.y1), ImVec2(0, 1), ImVec2(1, 0));

    // * Show gizmos
    gizmo_overlay_->draw_gizmo(render_surface_);
}

void SceneViewWidget::runtime_start()
{
    DLOGN("scene") << "Starting runtime." << std::endl;
    // Save main scene
    SceneManager::get("main_scene"_h).save();
    // Create a shallow copy of current scene (shared asset registry) and reload
    auto& runtime_scene = SceneManager::create_scene("runtime"_h);
    runtime_scene.runtime_clone(scn::current());
    SceneManager::make_current("runtime"_h);

    runtime_ = true;
}

void SceneViewWidget::runtime_stop()
{
    DLOGN("scene") << "Ending runtime." << std::endl;

    // Force unpause on exiting runtime (avoids weird bugs)
    paused_ = false;
    Application::get_instance().get_clock().pause(paused_);

    // Copy values of script parameters that may have been modified during runtime back to main scene
    if(cfg::get("settings.scripting.transport_runtime_parameters"_h, true))
    {
        auto target_context = SceneManager::get("main_scene"_h).get_script_context();
        auto source_context = SceneManager::get("runtime"_h).get_script_context();
        ScriptEngine::transport_runtime_parameters(source_context, target_context);
    }

    // Destroy runtime scene and jump back to editor scene
    SceneManager::make_current("main_scene"_h);
    SceneManager::remove_scene("runtime"_h);

    runtime_ = false;
}

void SceneViewWidget::runtime_pause()
{
    DLOGN("scene") << (paused_ ? "Resuming runtime." : "Pausing runtime.") << std::endl;
    paused_ = !paused_;
    Application::get_instance().get_clock().pause(paused_);
}

void SceneViewWidget::runtime_reset()
{
    DLOGN("scene") << "Resetting runtime." << std::endl;
    DLOGW("scene") << "Runtime reset feature not implemented yet." << std::endl;
}

static int s_profile_num_frames = 60;
static int s_frames_counter = 0;
static bool s_frame_profile_running = false;
void SceneViewWidget::frame_profiler_window(bool* p_open)
{
    if(ImGui::Begin("Profiling", p_open))
    {
        if(ImGui::Button("Runtime profiling"))
        {
            enable_runtime_profiling_ = !enable_runtime_profiling_;
            W_PROFILE_ENABLE_SESSION(enable_runtime_profiling_);
        }

        ImGui::SameLine();
        if(enable_runtime_profiling_ || s_frame_profile_running)
            ImGui::TextColored({0.f, 1.f, 0.f, 1.f}, "[*]");
        else
            ImGui::TextColored({1.f, 0.f, 0.f, 1.f}, "[ ]");

        ImGui::Text("Profile next frames");
        ImGui::SliderInt("Frames", &s_profile_num_frames, 1, 120);

        if(ImGui::Button("Start"))
        {
            s_frame_profile_running = !s_frame_profile_running;
            s_frames_counter = 0;
            W_PROFILE_ENABLE_SESSION(true);
        }

        ImGui::Separator();
        if(ImGui::Button("Track draw calls"))
            track_next_frame_draw_calls_ = true;

        ImGui::End();
    }
    if(s_frame_profile_running)
    {
        if(s_frames_counter++ == s_profile_num_frames)
        {
            s_frame_profile_running = false;
            W_PROFILE_ENABLE_SESSION(false);
        }
    }
}

} // namespace editor