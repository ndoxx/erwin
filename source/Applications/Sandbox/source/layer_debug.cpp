#include "layer_debug.h"

#include <iostream>
#include <iomanip>
#include <bitset>

using namespace erwin;

DebugLayer::DebugLayer(): Layer("DebugLayer")
{

}

void DebugLayer::on_imgui_render()
{
    ImGui::Begin("Debug");
#ifdef W_PROFILE
        if(ImGui::TreeNode("Profiling"))
        {
            if(ImGui::Button("Runtime profiling"))
            {
                enable_runtime_profiling_ = !enable_runtime_profiling_;
                W_PROFILE_ENABLE_SESSION(enable_runtime_profiling_);
            }

            ImGui::SameLine();
            if(enable_runtime_profiling_ || frame_profiling_)
                ImGui::TextColored({0.f,1.f,0.f,1.f}, "[*]");
            else
                ImGui::TextColored({1.f,0.f,0.f,1.f}, "[ ]");

            ImGui::Text("Profile next frames");
            ImGui::SliderInt("Frames", &profile_num_frames_, 1, 120);

            if(ImGui::Button("Start"))
            {
                frame_profiling_ = !frame_profiling_;
                frames_counter_ = 0;
                W_PROFILE_ENABLE_SESSION(true);
            }
        }

        if(frame_profiling_)
        {
            if(frames_counter_++ == profile_num_frames_)
            {
                frame_profiling_ = false;
                W_PROFILE_ENABLE_SESSION(false);
            }
        }
#endif
    ImGui::End();
}

void DebugLayer::on_attach()
{

}

void DebugLayer::on_detach()
{

}

void DebugLayer::on_update(GameClock& clock)
{

}

bool DebugLayer::on_event(const MouseButtonEvent& event)
{
	return false;
}

bool DebugLayer::on_event(const WindowResizeEvent& event)
{
	return false;
}

bool DebugLayer::on_event(const MouseScrollEvent& event)
{
	return false;
}
