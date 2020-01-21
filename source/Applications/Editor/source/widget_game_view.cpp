#include "widget_game_view.h"
#include "erwin.h"
#include "scene.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

GameViewWidget::GameViewWidget(Scene& scene):
Widget("Game", true),
scene_(scene)
{
	flags_ |= ImGuiWindowFlags_MenuBar;

    enable_runtime_profiling_ = cfg::get<bool>("erwin.profiling.runtime_session_enabled"_h, false);
}

GameViewWidget::~GameViewWidget()
{

}

static constexpr float k_border = 4.f;
static constexpr float k_start_x = 4.f;
static constexpr float k_start_y = 43.f;

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
        ImGui::EndMenuBar();
	}

    if(show_frame_profiler) frame_profiler_window(&show_frame_profiler);

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

        /*ImGui::Separator();
        if(ImGui::Button("Track draw calls"))
        {
            editor_.track_draw_calls("draw_calls.json");
        }*/

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