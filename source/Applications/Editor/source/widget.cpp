#include "widget.h"
#include "imgui.h"

namespace editor
{

Widget::Widget(const std::string& name, bool open):
open_(open),
name_(name)
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

    on_render();
    ImGui::End();
}


} // namespace editor