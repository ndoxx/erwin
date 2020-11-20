#include "widget/widget_console.h"
#include "script/script_engine.h"
#include "level/scene_manager.h"
#include <kibble/logger/logger.h>
#include "imgui.h"

#include <regex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

using namespace erwin;

namespace editor
{

static const std::map<char, ImVec4> s_color_table =
{
	{'N', ImVec4(0.59f, 0.51f, 1.f, 1.f)},
	{'W', ImVec4(1.f, 0.69f, 0.f, 1.f)},
	{'E', ImVec4(1.f, 0.35f, 0.35f, 1.f)},
	{'F', ImVec4(1.f, 0.f, 0.f, 1.f)},
	{'G', ImVec4(0.f, 1.f, 0.f, 1.f)},
	{'B', ImVec4(1.f, 0.f, 0.f, 1.f)}
};

static inline int text_edit_forward_callback(ImGuiInputTextCallbackData* data)
{
    return static_cast<ConsoleWidget*>(data->UserData)->text_edit_callback(data);
}

ConsoleWidget::ConsoleWidget():
Widget("Console", true)
{
	memset(input_buffer_, 0, sizeof(input_buffer_));
	auto_scroll_ = true;
	scroll_to_bottom_ = false;
	queue_max_len_ = 100;
	history_max_len_ = 50;
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

static const std::regex style_regex("\\[!(.)\\]");
void ConsoleWidget::push(const std::string& message)
{
	// Parse style
	std::smatch match;
	char style = 0;
	if(std::regex_search(message, match, style_regex))
		style = match.str(1)[0];

	items_.push_back({message, style});
    if(items_.size() == queue_max_len_)
       items_.pop_front();
}

void ConsoleWidget::send_command(const std::string& command)
{
	// Display in widget and evaluate in script engine
	auto ctx_handle = scn::current().get_script_context();
	push("ctx_" + std::to_string(ctx_handle) + "> " + command);
	save_command(command); // TODO: use command history to enable command navigation with arrow keys
	ScriptEngine::get_context(ctx_handle).eval(command);
}

void ConsoleWidget::save_command(const std::string& command)
{
	command_history_.push_back(command);
    if(command_history_.size() == history_max_len_)
       command_history_.pop_front();
}

void ConsoleWidget::on_imgui_render()
{
	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

	// Display all messages
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1)); // Tighten spacing
	for(const auto& item: items_)
	{
		bool pop_color = false;
		uint32_t offset = 0;

		if(item.style)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, s_color_table.at(item.style));
			pop_color = true;
			offset = 4;
		}

        ImGui::TextUnformatted(item.message.c_str() + offset);

        if(pop_color)
        	ImGui::PopStyleColor();
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
	   &text_edit_forward_callback, this))
	{
		send_command(std::string(input_buffer_));
		memset(input_buffer_, 0, sizeof(input_buffer_));
		reclaim_focus = true;
		scroll_to_bottom_ = true;
	}

    // Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if(reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
}


static const std::regex ansi_regex("\033\\[.+?m"); // matches ANSI codes
static std::string strip_ansi(const std::string& str)
{
	return std::regex_replace(str, ansi_regex, "");
}

static std::string message_type_to_style_tag(kb::klog::MsgType type)
{
	switch(type)
	{
		case kb::klog::MsgType::RAW:     return "";
	    case kb::klog::MsgType::NORMAL:  return "";
	    case kb::klog::MsgType::ITEM:    return "";
	    case kb::klog::MsgType::EVENT:   return "";
	    case kb::klog::MsgType::NOTIFY:  return "[!N]";
	    case kb::klog::MsgType::WARNING: return "[!W]";
	    case kb::klog::MsgType::ERROR:   return "[!E]";
	    case kb::klog::MsgType::FATAL:   return "[!F]";
	    case kb::klog::MsgType::BANG:    return "";
	    case kb::klog::MsgType::GOOD:    return "[!G]";
	    case kb::klog::MsgType::BAD:     return "[!B]";
	    default:                           return "";
	}
}

ConsoleWidgetSink::ConsoleWidgetSink(ConsoleWidget* p_console):
console_(p_console)
{

}

void ConsoleWidgetSink::send(const kb::klog::LogStatement& stmt, const kb::klog::LogChannel&)
{
	float ts = std::chrono::duration_cast<std::chrono::duration<float>>(stmt.timestamp).count();
	std::stringstream ss;
	ss << message_type_to_style_tag(stmt.msg_type)
	   << "[" << std::setprecision(6) << std::fixed << ts << "] " << strip_ansi(stmt.message);
	console_->push(ss.str());
}

void ConsoleWidgetSink::send_raw(const std::string& message)
{
	console_->push(message);
}

} // namespace editor