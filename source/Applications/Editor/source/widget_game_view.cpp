#include "widget_game_view.h"
#include "overlay_stats.h"
#include "overlay_camera_tracker.h"
#include "erwin.h"
#include "game/scene.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

static constexpr float k_border = 4.f;
static constexpr float k_start_x = 4.f;
static constexpr float k_start_y = 43.f;
static constexpr float k_overlay_dist = 10.f;

GameViewWidget::GameViewWidget(game::Scene& scene):
Widget("Game", true),
scene_(scene)
{
	flags_ |= ImGuiWindowFlags_MenuBar;
    enable_runtime_profiling_ = cfg::get<bool>("erwin.profiling.runtime_session_enabled"_h, false);
    track_next_frame_draw_calls_ = false;
    stats_overlay_ = new RenderStatsOverlay();
    camera_overlay_ = new CameraTrackerOverlay(scene_);
    render_surface_ = {0.f,0.f,0.f,0.f};
}

GameViewWidget::~GameViewWidget()
{
    delete stats_overlay_;
	delete camera_overlay_;
}

void GameViewWidget::on_update()
{
	stats_overlay_->on_update();

	if(track_next_frame_draw_calls_)
	{
		Renderer::track_draw_calls("draw_calls.json");
		track_next_frame_draw_calls_ = false;
	}
}

void GameViewWidget::on_resize(uint32_t width, uint32_t height)
{
	float rw = std::max(width  - (k_border + k_start_x), 0.f);
	float rh = std::max(height - (k_border + k_start_y), 0.f);
	render_surface_.x1 = render_surface_.x0 + rw;
	render_surface_.y1 = render_surface_.y0 + rh;

	EVENTBUS.publish(WindowResizeEvent(width, height));
	EVENTBUS.publish(FramebufferResizeEvent(rw, rh));
}

void GameViewWidget::on_move(int32_t x, int32_t y)
{
	render_surface_.x0 = x + k_start_x;
	render_surface_.y0 = y + k_start_y;

	EVENTBUS.publish(WindowMovedEvent(x, y));
}

bool GameViewWidget::on_event(const erwin::MouseButtonEvent& event)
{
    /*if(event.button == keymap::WMOUSE::BUTTON_0 &&
       event.x > render_surface_.x0 &&
       event.x < render_surface_.x1 &&
       event.y > render_surface_.y0 &&
       event.y < render_surface_.y1)
    {
        scene_.camera_controller.toggle_control();
        return true;
    }*/

    return false;
}

void GameViewWidget::on_imgui_render()
{
	static bool show_frame_profiler = false;
    if(ImGui::BeginMenuBar())
    {
	    if(ImGui::BeginMenu("Profiling"))
	    {
	        ImGui::MenuItem("Frame profiler", NULL, &show_frame_profiler);
	        ImGui::EndMenu();
	    }
        if(ImGui::BeginMenu("Overlays"))
        {
            ImGui::MenuItem("Render stats", NULL,   &stats_overlay_->open_);
            ImGui::MenuItem("Camera tracker", NULL, &camera_overlay_->open_);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
	}

    if(show_frame_profiler) frame_profiler_window(&show_frame_profiler);

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
	FramebufferHandle fb = FramebufferPool::get_framebuffer("game_view"_h);
	TextureHandle texture = Renderer::get_framebuffer_texture(fb, 0);
	void* framebuffer_texture_native = Renderer::get_native_texture_handle(texture);
    ImGui::GetWindowDrawList()->AddImage(framebuffer_texture_native,
                                         // ImGui::GetCursorScreenPos(),
                                         ImVec2(render_surface_.x0, render_surface_.y0),
                                         ImVec2(render_surface_.x1, render_surface_.y1),
                                         ImVec2(0, 1), ImVec2(1, 0));
}

void GameViewWidget::frame_profiler_window(bool* p_open)
{
	static int profile_num_frames = 60;
	static int frames_counter = 0;
	static bool frame_profile_running = false;

    if(ImGui::Begin("Profiling", p_open))
    {
        if(ImGui::Button("Runtime profiling"))
        {
            enable_runtime_profiling_ = !enable_runtime_profiling_;
            W_PROFILE_ENABLE_SESSION(enable_runtime_profiling_);
        }

        ImGui::SameLine();
        if(enable_runtime_profiling_ || frame_profile_running)
            ImGui::TextColored({0.f,1.f,0.f,1.f}, "[*]");
        else
            ImGui::TextColored({1.f,0.f,0.f,1.f}, "[ ]");

        ImGui::Text("Profile next frames");
        ImGui::SliderInt("Frames", &profile_num_frames, 1, 120);

        if(ImGui::Button("Start"))
        {
            frame_profile_running = !frame_profile_running;
            frames_counter = 0;
            W_PROFILE_ENABLE_SESSION(true);
        }

        ImGui::Separator();
        if(ImGui::Button("Track draw calls"))
        	track_next_frame_draw_calls_ = true;

    	ImGui::End();
    }
    if(frame_profile_running)
    {
        if(frames_counter++ == profile_num_frames)
        {
            frame_profile_running = false;
            W_PROFILE_ENABLE_SESSION(false);
        }
    }
}

} // namespace editor