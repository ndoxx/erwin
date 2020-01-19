#include "widget_console.h"
#include "debug/logger.h"
#include "imgui.h"

#include <regex>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace editor
{

ConsoleWidget::ConsoleWidget():
Widget("Console", true)
{
	memset(input_buffer_, 0, sizeof(input_buffer_));
	auto_scroll_ = true;
	scroll_to_bottom_ = false;
}

ConsoleWidget::~ConsoleWidget()
{

}

static int text_edit_forward_callback(ImGuiInputTextCallbackData* data)
{
    ConsoleWidget* console = (ConsoleWidget*)data->UserData;
    return console->text_edit_callback(data);
}

int ConsoleWidget::text_edit_callback(void* _data)
{
	ImGuiInputTextCallbackData* data = static_cast<ImGuiInputTextCallbackData*>(_data);

	switch (data->EventFlag)
	{
		case ImGuiInputTextFlags_CallbackCompletion:
	    {

	    	break;
	    }
        case ImGuiInputTextFlags_CallbackHistory:
	    {

	    	break;
	    }
	}

	return 0;
}

void ConsoleWidget::push(const std::string& message)
{
	items_.push_back(message);
}

void ConsoleWidget::on_render()
{
	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

	// Display all messages
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1)); // Tighten spacing
	for(const std::string& item: items_)
	{
        ImGui::TextUnformatted(item.c_str());
	}

	if(scroll_to_bottom_ || (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
		ImGui::SetScrollHereY(1.0f);
	scroll_to_bottom_ = false;

	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::Separator();

    // Command-line
    bool reclaim_focus = false;
	if(ImGui::InputText("Input", input_buffer_, IM_ARRAYSIZE(input_buffer_), 
	   ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_CallbackHistory,
	   &text_edit_forward_callback, (void*)this))
	{
		char* s = input_buffer_;
		strcpy(s, "");
		reclaim_focus = true;
		scroll_to_bottom_ = true;
	}

    // Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if(reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
}

static std::string strip_ansi(const std::string& str)
{
	static std::regex ansi_regex("\033\\[.+?m"); // matches ANSI codes
	return std::regex_replace(str, ansi_regex, "");
}

ConsoleWidgetSink::ConsoleWidgetSink(ConsoleWidget* p_console):
console_(p_console)
{

}

void ConsoleWidgetSink::send(const erwin::dbg::LogStatement& stmt, const erwin::dbg::LogChannel& chan)
{
	float ts = std::chrono::duration_cast<std::chrono::duration<float>>(stmt.timestamp).count();
	std::stringstream ss;
	ss << "[" << std::setprecision(6) << std::fixed << ts << "] " << strip_ansi(stmt.message);
	console_->push(ss.str());
}

void ConsoleWidgetSink::send_raw(const std::string& message)
{
	console_->push(message);
}

} // namespace editor