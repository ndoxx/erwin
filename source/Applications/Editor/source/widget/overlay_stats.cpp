#include "widget/overlay_stats.h"
#include "erwin.h"
#include "imgui.h"
#include "imgui/imgui_utils.h"
#include "level/scene.h"

using namespace erwin;

namespace editor
{

RenderStatsOverlay::RenderStatsOverlay(erwin::EventBus& event_bus) : Widget("Statistics", false, event_bus)
{
    flags_ = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav;
}

void RenderStatsOverlay::on_update(const GameClock&)
{
#ifdef W_DEBUG
    Renderer::set_profiling_enabled(open_);
#endif
}

void RenderStatsOverlay::on_imgui_render()
{
#ifdef W_DEBUG
    const auto& r_stats = Renderer::get_stats();

    ImGui::Text("Draw calls: %d", r_stats.draw_call_count);
    ImGui::Separator();
    ImGui::PlotVar("GPU Draw (µs)", r_stats.GPU_render_time, 0.0f, 7000.f);
    ImGui::PlotVar("CPU Flush (µs)", r_stats.CPU_flush_time, 0.0f, 7000.f);
    ImGui::PlotVarFlushOldEntries();
#endif
}

} // namespace editor