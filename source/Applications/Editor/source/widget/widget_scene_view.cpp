#include "widget/widget_scene_view.h"
#include "widget/overlay_stats.h"
#include "widget/overlay_camera_tracker.h"
#include "level/scene.h"
#include "core/config.h"
#include "core/application.h"
#include "event/event_bus.h"
#include "event/window_events.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"
#include "entity/reflection.h"
#include "entity/component_bounding_box.h"
#include "entity/component_transform.h"
#include "asset/material.h"
#include "asset/bounding.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

static constexpr float k_border = 4.f;
static constexpr float k_start_x = 4.f;
static constexpr float k_start_y = 43.f;
static constexpr float k_overlay_dist = 10.f;

SceneViewWidget::SceneViewWidget():
Widget("Scene", true),
render_surface_{0.f,0.f,0.f,0.f,0.f,0.f}
{
	flags_ |= ImGuiWindowFlags_MenuBar;
    enable_runtime_profiling_ = cfg::get<bool>("erwin.profiling.runtime_session_enabled"_h, false);
    track_next_frame_draw_calls_ = false;
    stats_overlay_ = new RenderStatsOverlay();
    camera_overlay_ = new CameraTrackerOverlay();
}

SceneViewWidget::~SceneViewWidget()
{
    delete stats_overlay_;
	delete camera_overlay_;
}

void SceneViewWidget::on_update(const GameClock& clock)
{
	stats_overlay_->on_update(clock);

	if(track_next_frame_draw_calls_)
	{
		Renderer::track_draw_calls("draw_calls.json");
		track_next_frame_draw_calls_ = false;
	}
}

void SceneViewWidget::on_resize(uint32_t width, uint32_t height)
{
	float rw = std::max(float(width)  - (k_border + k_start_x), 1.f);
	float rh = std::max(float(height) - (k_border + k_start_y), 1.f);
	render_surface_.x1 = render_surface_.x0 + rw;
	render_surface_.y1 = render_surface_.y0 + rh;
    render_surface_.w = rw;
    render_surface_.h = rh;

	EventBus::publish(WindowResizeEvent(int(width), int(height)));
	EventBus::publish(FramebufferResizeEvent(int(rw), int(rh)));
}

void SceneViewWidget::on_move(int32_t x, int32_t y)
{
    float rw = std::max(float(width_)  - (k_border + k_start_x), 1.f);
    float rh = std::max(float(height_) - (k_border + k_start_y), 1.f);
	render_surface_.x0 = float(x) + k_start_x;
	render_surface_.y0 = float(y) + k_start_y;
    render_surface_.x1 = render_surface_.x0 + rw;
    render_surface_.y1 = render_surface_.y0 + rh;

	EventBus::publish(WindowMovedEvent(x, y));
}

bool SceneViewWidget::on_mouse_event(const erwin::MouseButtonEvent& event)
{
    if(event.button == keymap::WMOUSE::BUTTON_0 &&
       event.pressed &&
       event.x > render_surface_.x0 &&
       event.x < render_surface_.x1 &&
       event.y > render_surface_.y0 &&
       event.y < render_surface_.y1)
    {
        glm::vec2 coords = {     (event.x - render_surface_.x0)/render_surface_.w, 
                             1.f-(event.y - render_surface_.y0)/render_surface_.h };

        EventBus::publish(RaySceneQueryEvent(coords));

        return true;
    }

    return false;
}

static bool s_show_frame_profiler = false;
static bool s_enable_debug_show_uv = false;
void SceneViewWidget::on_imgui_render()
{
    if(ImGui::BeginMenuBar())
    {
	    if(ImGui::BeginMenu("Profiling"))
	    {
	        ImGui::MenuItem("Frame profiler", NULL, &s_show_frame_profiler);
	        ImGui::EndMenu();
	    }
        if(ImGui::BeginMenu("Overlays"))
        {
            ImGui::MenuItem("Render stats", NULL,   &stats_overlay_->open_);
            ImGui::MenuItem("Camera tracker", NULL, &camera_overlay_->open_);
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Debug"))
        {
            if(ImGui::MenuItem("Show UV", NULL, &s_enable_debug_show_uv))
                Renderer3D::debug_show_uv(s_enable_debug_show_uv);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
	}

    if(s_show_frame_profiler) frame_profiler_window(&s_show_frame_profiler);

    if(stats_overlay_->open_)
    {
		ImVec2 overlay_pos(render_surface_.x1 - k_overlay_dist, render_surface_.y0 + k_overlay_dist);
		ImVec2 overlay_pivot(1.f, 0.f);
		ImGui::SetNextWindowPos(overlay_pos, ImGuiCond_Always, overlay_pivot);
		ImGui::SetNextWindowBgAlpha(0.35f);
	    stats_overlay_->imgui_render();
	}

    if(camera_overlay_->open_)
    {
        ImVec2 overlay_pos(render_surface_.x0 + k_overlay_dist, render_surface_.y0 + k_overlay_dist);
        ImVec2 overlay_pivot(0.f, 0.f);
        ImGui::SetNextWindowPos(overlay_pos, ImGuiCond_Always, overlay_pivot);
        ImGui::SetNextWindowBgAlpha(0.35f);
        camera_overlay_->imgui_render();
    }

    // * Show game render in window
	// Retrieve the native framebuffer texture handle
	FramebufferHandle fb = FramebufferPool::get_framebuffer("host"_h);
	TextureHandle texture = Renderer::get_framebuffer_texture(fb, 0);
	void* framebuffer_texture_native = Renderer::get_native_texture_handle(texture);
    ImGui::GetWindowDrawList()->AddImage(framebuffer_texture_native,
                                         // ImGui::GetCursorScreenPos(),
                                         ImVec2(render_surface_.x0, render_surface_.y0),
                                         ImVec2(render_surface_.x1, render_surface_.y1),
                                         ImVec2(0, 1), ImVec2(1, 0));
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
            ImGui::TextColored({0.f,1.f,0.f,1.f}, "[*]");
        else
            ImGui::TextColored({1.f,0.f,0.f,1.f}, "[ ]");

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