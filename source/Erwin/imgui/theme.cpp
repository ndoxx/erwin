#include "imgui/theme.h"
#include "filesystem/xml_file.h"
#include "imgui.h"
#include "debug/logger.h"

using namespace erwin;

namespace editor
{

static std::map<hash_t, ImGuiCol_> s_imgui_col_map =
{
    {"ImGuiCol_Text"_h,					ImGuiCol_Text},
    {"ImGuiCol_TextDisabled"_h,			ImGuiCol_TextDisabled},
    {"ImGuiCol_WindowBg"_h,				ImGuiCol_WindowBg},
    {"ImGuiCol_ChildBg"_h,				ImGuiCol_ChildBg},
    {"ImGuiCol_PopupBg"_h,				ImGuiCol_PopupBg},
    {"ImGuiCol_Border"_h,				ImGuiCol_Border},
    {"ImGuiCol_BorderShadow"_h,			ImGuiCol_BorderShadow},
    {"ImGuiCol_FrameBg"_h,				ImGuiCol_FrameBg},
    {"ImGuiCol_FrameBgHovered"_h,		ImGuiCol_FrameBgHovered},
    {"ImGuiCol_FrameBgActive"_h,		ImGuiCol_FrameBgActive},
    {"ImGuiCol_TitleBg"_h,				ImGuiCol_TitleBg},
    {"ImGuiCol_TitleBgActive"_h,		ImGuiCol_TitleBgActive},
    {"ImGuiCol_TitleBgCollapsed"_h,		ImGuiCol_TitleBgCollapsed},
    {"ImGuiCol_MenuBarBg"_h,			ImGuiCol_MenuBarBg},
    {"ImGuiCol_ScrollbarBg"_h,			ImGuiCol_ScrollbarBg},
    {"ImGuiCol_ScrollbarGrab"_h,		ImGuiCol_ScrollbarGrab},
    {"ImGuiCol_ScrollbarGrabHovered"_h,	ImGuiCol_ScrollbarGrabHovered},
    {"ImGuiCol_ScrollbarGrabActive"_h,	ImGuiCol_ScrollbarGrabActive},
    {"ImGuiCol_CheckMark"_h,			ImGuiCol_CheckMark},
    {"ImGuiCol_SliderGrab"_h,			ImGuiCol_SliderGrab},
    {"ImGuiCol_SliderGrabActive"_h,		ImGuiCol_SliderGrabActive},
    {"ImGuiCol_Button"_h,				ImGuiCol_Button},
    {"ImGuiCol_ButtonHovered"_h,		ImGuiCol_ButtonHovered},
    {"ImGuiCol_ButtonActive"_h,			ImGuiCol_ButtonActive},
    {"ImGuiCol_Header"_h,				ImGuiCol_Header},
    {"ImGuiCol_HeaderHovered"_h,		ImGuiCol_HeaderHovered},
    {"ImGuiCol_HeaderActive"_h,			ImGuiCol_HeaderActive},
    {"ImGuiCol_Separator"_h,			ImGuiCol_Separator},
    {"ImGuiCol_SeparatorHovered"_h,		ImGuiCol_SeparatorHovered},
    {"ImGuiCol_SeparatorActive"_h,		ImGuiCol_SeparatorActive},
    {"ImGuiCol_ResizeGrip"_h,			ImGuiCol_ResizeGrip},
    {"ImGuiCol_ResizeGripHovered"_h,	ImGuiCol_ResizeGripHovered},
    {"ImGuiCol_ResizeGripActive"_h,		ImGuiCol_ResizeGripActive},
    {"ImGuiCol_Tab"_h,					ImGuiCol_Tab},
    {"ImGuiCol_TabHovered"_h,			ImGuiCol_TabHovered},
    {"ImGuiCol_TabActive"_h,			ImGuiCol_TabActive},
    {"ImGuiCol_TabUnfocused"_h,			ImGuiCol_TabUnfocused},
    {"ImGuiCol_TabUnfocusedActive"_h,	ImGuiCol_TabUnfocusedActive},
    {"ImGuiCol_DockingPreview"_h,		ImGuiCol_DockingPreview},
    {"ImGuiCol_DockingEmptyBg"_h,		ImGuiCol_DockingEmptyBg},
    {"ImGuiCol_PlotLines"_h,			ImGuiCol_PlotLines},
    {"ImGuiCol_PlotLinesHovered"_h,		ImGuiCol_PlotLinesHovered},
    {"ImGuiCol_PlotHistogram"_h,		ImGuiCol_PlotHistogram},
    {"ImGuiCol_PlotHistogramHovered"_h,	ImGuiCol_PlotHistogramHovered},
    {"ImGuiCol_TextSelectedBg"_h,		ImGuiCol_TextSelectedBg},
    {"ImGuiCol_DragDropTarget"_h,		ImGuiCol_DragDropTarget},
    {"ImGuiCol_NavHighlight"_h,			ImGuiCol_NavHighlight},
    {"ImGuiCol_NavWindowingHighlight"_h,ImGuiCol_NavWindowingHighlight},
    {"ImGuiCol_NavWindowingDimBg"_h,	ImGuiCol_NavWindowingDimBg},
    {"ImGuiCol_ModalWindowDimBg"_h,		ImGuiCol_ModalWindowDimBg},
};

bool load_theme(const fs::path& xml_path)
{
	DLOGN("editor") << "Loading theme: " << std::endl;
	DLOGI << WCC('p') << xml_path.filename() << std::endl;

	xml::XMLFile theme_f(xml_path);
	if(!theme_f.read())
		return false;

	// TMP
	ImGui::StyleColorsDark();

	auto* root = theme_f.root;
	if(!root)
		return false;

	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();

	auto* window_rounding_node = root->first_node("WindowRounding");
	if(window_rounding_node)
	{
		float window_rounding = 5.f;
		xml::parse_attribute(window_rounding_node, "value", window_rounding);

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		else
		{
	    	style.WindowRounding = window_rounding;
		}
	}

    for(rapidxml::xml_node<>* col_node=root->first_node("vec4");
        col_node; col_node=col_node->next_sibling("vec4"))
    {
    	hash_t hname = xml::parse_attribute_h(col_node, "name");
    	auto it = s_imgui_col_map.find(hname);
    	if(it == s_imgui_col_map.end())
    	{
    		std::string name;
    		xml::parse_attribute(col_node, "name", name);
    		DLOGW("editor") << "Skipping unknown ImGuiCol: " << name << std::endl;
    		continue;
    	}

    	glm::vec4 color;
    	if(xml::parse_attribute(col_node, "value", color))
    	{
    		ImVec4 im_color = {color.x, color.y, color.z, color.w};
    		auto enum_col = it->second;
    		style.Colors[enum_col] = im_color;
    	}
    }

	return true;
}

} // namespace editor