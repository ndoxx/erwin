#include "editor/widget.h"
#include "debug/logger.h"
#include "imgui.h"

namespace editor
{

Widget::Widget(const std::string& name, bool open):
open_(open),
name_(name),
flags_(0),
width_(0),
height_(0),
x_pos_(0),
y_pos_(0),
has_focus_(false)
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

    ImVec2 window_pos = ImGui::GetWindowPos();
    if(window_pos.x != x_pos_ || window_pos.y != y_pos_)
    {
        on_move(window_pos.x, window_pos.y);
        x_pos_ = window_pos.x;
        y_pos_ = window_pos.y;
    }

    ImVec2 window_size = ImGui::GetWindowSize();
    if(window_size.x != width_ || window_size.y != height_)
    {
    	on_resize(window_size.x, window_size.y);
    	width_ = window_size.x;
    	height_ = window_size.y;
    }

    has_focus_ = ImGui::IsWindowFocused();

    on_imgui_render();
    ImGui::End();
}


} // namespace editor