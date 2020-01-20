#include "widget.h"
#include "imgui.h"
#include "debug/logger.h"

namespace editor
{

Widget::Widget(const std::string& name, bool open):
open_(open),
name_(name),
width_(0),
height_(0)
{

}

void Widget::render()
{
	if(!open_)
		return;

    if(!ImGui::Begin(name_.c_str(), &open_))
    {
    	ImGui::End();
    	return;
    }

    ImVec2 window_size = ImGui::GetWindowSize();
    if(window_size.x != width_ || window_size.y != height_)
    {
    	on_resize(window_size.x, window_size.y);
    	width_ = window_size.x;
    	height_ = window_size.y;
    }

    on_render();
    ImGui::End();
}


} // namespace editor