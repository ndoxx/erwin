#pragma once

#include <limits>

namespace ImGui
{
// Plot value over time
// Pass FLT_MAX value to draw without adding a new value
void PlotVar(const char* label,
             float value,
             float scale_min = std::numeric_limits<float>::max(),
             float scale_max = std::numeric_limits<float>::max(),
             size_t buffer_size = 200);

// Call this periodically to discard old/unused data
void PlotVarFlushOldEntries();

void WCombo(const char* combo_name, const char* text, int& current_index, int nitems, const char** items);

bool SliderFloatDefault(const char* label, float* v, float v_min, float v_max, float v_default, const char* display_format = "%.3f");

}
