#include "widget/widget.h"
#include "event/event_bus.h"
#include <kibble/logger/logger.h>
#include "imgui.h"

namespace editor
{

Widget::Widget(const std::string& name, bool open, erwin::EventBus& event_bus):
open_(open),
name_(name),
dbg_profile_name_("imgui_render: " + name_),
flags_(0),
width_(0),
height_(0),
x_pos_(0),
y_pos_(0),
has_focus_(false),
is_hovered_(false),
was_open_(open),
event_bus_(event_bus)
{
    
}

void Widget::imgui_render()
{
	if(!open_)
		return;

    if(!ImGui::Begin(name_.c_str(), &open_, flags_))
    {
    	ImGui::End();
    	return;
    }
    W_PROFILE_SCOPE(dbg_profile_name_.c_str());

    ImVec2 window_pos = ImGui::GetWindowPos();
    int32_t win_pos_x = int32_t(window_pos.x);
    int32_t win_pos_y = int32_t(window_pos.y);
    if(win_pos_x != x_pos_ || win_pos_y != y_pos_)
    {
        on_move(win_pos_x, win_pos_y);
        x_pos_ = win_pos_x;
        y_pos_ = win_pos_y;
    }

    ImVec2 window_size = ImGui::GetWindowSize();
    uint32_t win_size_x = uint32_t(window_size.x);
    uint32_t win_size_y = uint32_t(window_size.y);
    if(win_size_x != width_ || win_size_y != height_)
    {
    	on_resize(win_size_x, win_size_y);
    	width_ = win_size_x;
    	height_ = win_size_y;
    }

    has_focus_ = ImGui::IsWindowFocused();
    is_hovered_ = ImGui::IsWindowHovered();

    on_imgui_render();
    ImGui::End();
}


} // namespace editor