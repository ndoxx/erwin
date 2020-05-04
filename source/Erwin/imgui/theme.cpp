#include "imgui/theme.h"
#include "filesystem/filesystem.h"
#include "filesystem/xml_file.h"
#include "imgui.h"
#include "debug/logger.h"

#include <algorithm>

using namespace erwin;

namespace editor
{
namespace theme
{

static const std::map<hash_t, ImGuiCol_> s_imgui_col_map =
{
    {"Text"_h,					ImGuiCol_Text},
    {"TextDisabled"_h,			ImGuiCol_TextDisabled},
    {"WindowBg"_h,				ImGuiCol_WindowBg},
    {"ChildBg"_h,				ImGuiCol_ChildBg},
    {"PopupBg"_h,				ImGuiCol_PopupBg},
    {"Border"_h,				ImGuiCol_Border},
    {"BorderShadow"_h,			ImGuiCol_BorderShadow},
    {"FrameBg"_h,				ImGuiCol_FrameBg},
    {"FrameBgHovered"_h,		ImGuiCol_FrameBgHovered},
    {"FrameBgActive"_h,		    ImGuiCol_FrameBgActive},
    {"TitleBg"_h,				ImGuiCol_TitleBg},
    {"TitleBgActive"_h,		    ImGuiCol_TitleBgActive},
    {"TitleBgCollapsed"_h,		ImGuiCol_TitleBgCollapsed},
    {"MenuBarBg"_h,			    ImGuiCol_MenuBarBg},
    {"ScrollbarBg"_h,			ImGuiCol_ScrollbarBg},
    {"ScrollbarGrab"_h,		    ImGuiCol_ScrollbarGrab},
    {"ScrollbarGrabHovered"_h,	ImGuiCol_ScrollbarGrabHovered},
    {"ScrollbarGrabActive"_h,	ImGuiCol_ScrollbarGrabActive},
    {"CheckMark"_h,			    ImGuiCol_CheckMark},
    {"SliderGrab"_h,			ImGuiCol_SliderGrab},
    {"SliderGrabActive"_h,		ImGuiCol_SliderGrabActive},
    {"Button"_h,				ImGuiCol_Button},
    {"ButtonHovered"_h,		    ImGuiCol_ButtonHovered},
    {"ButtonActive"_h,			ImGuiCol_ButtonActive},
    {"Header"_h,				ImGuiCol_Header},
    {"HeaderHovered"_h,		    ImGuiCol_HeaderHovered},
    {"HeaderActive"_h,			ImGuiCol_HeaderActive},
    {"Separator"_h,			    ImGuiCol_Separator},
    {"SeparatorHovered"_h,		ImGuiCol_SeparatorHovered},
    {"SeparatorActive"_h,		ImGuiCol_SeparatorActive},
    {"ResizeGrip"_h,			ImGuiCol_ResizeGrip},
    {"ResizeGripHovered"_h,	    ImGuiCol_ResizeGripHovered},
    {"ResizeGripActive"_h,		ImGuiCol_ResizeGripActive},
    {"Tab"_h,					ImGuiCol_Tab},
    {"TabHovered"_h,			ImGuiCol_TabHovered},
    {"TabActive"_h,			    ImGuiCol_TabActive},
    {"TabUnfocused"_h,			ImGuiCol_TabUnfocused},
    {"TabUnfocusedActive"_h,	ImGuiCol_TabUnfocusedActive},
    {"DockingPreview"_h,		ImGuiCol_DockingPreview},
    {"DockingEmptyBg"_h,		ImGuiCol_DockingEmptyBg},
    {"PlotLines"_h,			    ImGuiCol_PlotLines},
    {"PlotLinesHovered"_h,		ImGuiCol_PlotLinesHovered},
    {"PlotHistogram"_h,		    ImGuiCol_PlotHistogram},
    {"PlotHistogramHovered"_h,	ImGuiCol_PlotHistogramHovered},
    {"TextSelectedBg"_h,		ImGuiCol_TextSelectedBg},
    {"DragDropTarget"_h,		ImGuiCol_DragDropTarget},
    {"NavHighlight"_h,			ImGuiCol_NavHighlight},
    {"NavWindowingHighlight"_h, ImGuiCol_NavWindowingHighlight},
    {"NavWindowingDimBg"_h,	    ImGuiCol_NavWindowingDimBg},
    {"ModalWindowDimBg"_h,		ImGuiCol_ModalWindowDimBg},
};

static std::vector<ThemeEntry> s_themes;
static ImGuiStyle s_default_style;

static void parse_property(rapidxml::xml_node<>* prop_node, void* destination)
{
    hash_t htype = xml::parse_attribute_h(prop_node, "type");

    if(htype == 0) 
        return;

    switch(htype)
    {
        case "float"_h:
        {
            float value = 0.f;
            if(xml::parse_attribute(prop_node, "value", value))
                memcpy(destination, &value, sizeof(float));
            break;
        }
        case "vec2"_h:
        {
            glm::vec2 value;
            if(xml::parse_attribute(prop_node, "value", value))
                memcpy(destination, &value[0], 2*sizeof(float));
            break;
        }
        default: return;
    }
}

void init()
{
    // Iterate themes directory
    fs::path theme_dir = filesystem::get_system_asset_dir() / "themes";
    for(auto& entry: fs::directory_iterator(theme_dir))
    {
        if(!entry.is_regular_file())
            continue;
        if(entry.path().extension().string().compare(".xml"))
            continue;

        xml::XMLFile theme_f(entry.path());
        if(!theme_f.read())
            continue;
        auto* root = theme_f.root;
        if(!root)
            continue;

        std::string theme_name;
        if(!xml::parse_attribute(root, "name", theme_name))
            continue;

        hash_t hname = H_(theme_name.c_str());
        size_t index = s_themes.size();
        s_themes.push_back({theme_name, entry.path()});
        if(hname=="Default"_h)
        {
            // Force default theme at index 0
            std::swap(s_themes[0], s_themes[index]);
            index = 0;
        }
    }

    // Sort themes alphabetically (default theme stays at index 0)
    std::sort(s_themes.begin()+1, s_themes.end(), [](const ThemeEntry& a, const ThemeEntry& b)
    {
        return a.name < b.name;
    });

    // Also, save default style
    s_default_style = ImGui::GetStyle();
}

const std::vector<ThemeEntry>& get_list()
{
    return s_themes;
}

bool load(const ThemeEntry& entry)
{
    const auto& xml_path = entry.path;

	DLOGN("editor") << "Loading theme: " << entry.name << std::endl;

	xml::XMLFile theme_f(xml_path);
	if(!theme_f.read())
		return false;

	auto* root = theme_f.root;
	if(!root)
		return false;

    // Restore defaults
    reset();

    // Parse base coloring scheme if any
    hash_t hbase = xml::parse_attribute_h(root, "base");
    switch(hbase)
    {
        case "dark"_h:    ImGui::StyleColorsDark(); break;
        case "classic"_h: ImGui::StyleColorsClassic(); break;
        case "light"_h:   ImGui::StyleColorsLight(); break;
    }

	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();

    // Parse style colors
    for(rapidxml::xml_node<>* col_node=root->first_node("color");
        col_node; col_node=col_node->next_sibling("color"))
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

    // Parse style variables
    const std::map<hash_t, void*> prop_map =
    {
        {"Alpha"_h,               &style.Alpha},
        {"WindowPadding"_h,       &style.WindowPadding[0]},
        {"WindowRounding"_h,      &style.WindowRounding},
        {"WindowBorderSize"_h,    &style.WindowBorderSize},
        {"WindowMinSize"_h,       &style.WindowMinSize[0]},
        {"WindowTitleAlign"_h,    &style.WindowTitleAlign[0]},
        {"ChildRounding"_h,       &style.ChildRounding},
        {"ChildBorderSize"_h,     &style.ChildBorderSize},
        {"PopupRounding"_h,       &style.PopupRounding},
        {"PopupBorderSize"_h,     &style.PopupBorderSize},
        {"FramePadding"_h,        &style.FramePadding[0]},
        {"FrameRounding"_h,       &style.FrameRounding},
        {"FrameBorderSize"_h,     &style.FrameBorderSize},
        {"ItemSpacing"_h,         &style.ItemSpacing[0]},
        {"ItemInnerSpacing"_h,    &style.ItemInnerSpacing[0]},
        {"IndentSpacing"_h,       &style.IndentSpacing},
        {"ScrollbarSize"_h,       &style.ScrollbarSize},
        {"ScrollbarRounding"_h,   &style.ScrollbarRounding},
        {"GrabMinSize"_h,         &style.GrabMinSize},
        {"GrabRounding"_h,        &style.GrabRounding},
        {"TabRounding"_h,         &style.TabRounding},
        {"ButtonTextAlign"_h,     &style.ButtonTextAlign[0]},
        {"SelectableTextAlign"_h, &style.SelectableTextAlign[0]},
    };

    for(rapidxml::xml_node<>* prop_node=root->first_node("prop");
        prop_node; prop_node=prop_node->next_sibling("prop"))
    {
        hash_t hname = xml::parse_attribute_h(prop_node, "name");
        auto it = prop_map.find(hname);
        if(it == prop_map.end())
            continue;

        parse_property(prop_node, it->second);
    }

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    return true;
}

bool load_default()
{
    W_ASSERT(s_themes.size()!=0, "Themes list is empty, call theme::init().");
    return load(s_themes[0]);
}

void reset()
{
    ImGui::GetStyle() = s_default_style;
}

} // namespace theme
} // namespace editor