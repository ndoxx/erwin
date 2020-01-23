#include "widget_stats.h"
#include "game/scene.h"
#include "erwin.h"
#include "imgui.h"
#include "imgui/imgui_utils.h"

using namespace erwin;

namespace editor
{

RenderStatsWidget::RenderStatsWidget():
Widget("Statistics", true)
{
    flags_ = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar 
           | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize 
           | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing 
           | ImGuiWindowFlags_NoNav;
}

RenderStatsWidget::~RenderStatsWidget()
{

}

void RenderStatsWidget::on_update()
{
    Renderer::set_profiling_enabled(open_);
}

void RenderStatsWidget::on_imgui_render()
{
    const auto& r_stats = Renderer::get_stats();

    ImGui::Text("Draw calls: %d", r_stats.draw_call_count);
    ImGui::Separator();
    ImGui::PlotVar("GPU Draw (µs)", r_stats.GPU_render_time, 0.0f, 7000.f);
    ImGui::PlotVar("CPU Flush (µs)", r_stats.CPU_flush_time, 0.0f, 7000.f);
    ImGui::PlotVarFlushOldEntries();
}


} // namespace editor