#include "widget/widget_hex_dump.h"
#include "core/application.h"
#include "imgui.h"
#include "imgui/font_awesome.h"
#include <kibble/memory/memory.inc>

#include <iomanip>
#include <sstream>

using namespace erwin;
using namespace kb;

namespace editor
{

#ifdef W_DEBUG
using BlockDescriptions = std::vector<kb::memory::debug::AreaItem>;

static struct
{
    std::vector<BlockDescriptions> area_descriptions_;
    std::map<uint32_t, std::string> names_;
    uint32_t current_area_;
    size_t current_block_;
} s_storage;

HexDumpWidget::HexDumpWidget() : Widget("Hex dump", false) {}

void HexDumpWidget::refresh()
{
    s_storage.names_.clear();
    s_storage.area_descriptions_.clear();
    s_storage.current_area_ = 0;
    s_storage.current_block_ = 0;
    register_area_description("client", Application::get_client_area().get_block_descriptions());
    register_area_description("system", Application::get_system_area().get_block_descriptions());
    register_area_description("render", Application::get_render_area().get_block_descriptions());
}

void HexDumpWidget::register_area_description(const std::string& name,
                                              const std::vector<kb::memory::debug::AreaItem>& items)
{
    if(!items.empty())
    {
        s_storage.names_.insert(std::make_pair(s_storage.area_descriptions_.size(), name));
        s_storage.area_descriptions_.push_back(items);
    }
}

void HexDumpWidget::on_imgui_render()
{
    if(s_storage.names_.empty())
        return;

    static const char* cur_area = s_storage.names_[0].data();
    static const char* cur_block = s_storage.area_descriptions_[0][0].name.data();

    if(ImGui::BeginCombo("Area", cur_area))
    {
        for(unsigned int ii = 0; ii < s_storage.area_descriptions_.size(); ++ii)
        {
            bool is_selected = (cur_area == s_storage.names_[ii].data());

            if(ImGui::Selectable(s_storage.names_[ii].data(), is_selected))
            {
                cur_area = s_storage.names_[ii].data();
                s_storage.current_area_ = ii;
                s_storage.current_block_ = 0;
                cur_block = s_storage.area_descriptions_[ii][0].name.data();
            }
            if(is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if(ImGui::Button("Refresh"))
    {
        refresh();
        cur_area = s_storage.names_[s_storage.current_area_].data();
        cur_block = s_storage.area_descriptions_[s_storage.current_area_][0].name.data();
    }

    const BlockDescriptions& bd = s_storage.area_descriptions_[s_storage.current_area_];

    if(ImGui::BeginCombo("Block", cur_block))
    {
        for(size_t ii = 0; ii < bd.size(); ++ii)
        {
            bool is_selected = (cur_block == bd[ii].name.data());

            if(ImGui::Selectable(bd[ii].name.data(), is_selected))
            {
                s_storage.current_block_ = ii;
                cur_block = bd[ii].name.data();
            }
            if(is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    const kb::memory::debug::AreaItem& item = bd[s_storage.current_block_];

    const uint8_t* begin = static_cast<const uint8_t*>(item.begin);
    const uint8_t* end = static_cast<const uint8_t*>(item.end);
    uint8_t* current = const_cast<uint8_t*>(begin);

    std::stringstream ss;
    ss << std::hex;
    uint32_t page_size = 0;
    ImGui::Columns(2, "##hex-ascii");
    while(current < end && page_size < 1_kB)
    {
        ss << "[0x" << std::setfill('0') << std::setw(8) << std::size_t(current) << "]";

        ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.f, 1.f), "%s", ss.str().c_str());
        ss.str("");

        for(int ii = 0; ii < 8; ++ii)
        {
            uint8_t value = *current;
            ss << std::setfill('0') << std::setw(2) << int(value) << (ii == 3 ? "  " : " ");

            ++current;
            ++page_size;
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(ss.str().c_str());
        ss.str("");
    }

    ImGui::NextColumn();
    current = const_cast<uint8_t*>(begin);
    page_size = 0;
    while(current < end && page_size < 1_kB)
    {
        for(int ii = 0; ii < 8; ++ii)
        {
            char value = char(*current);
            value = (value < 33 || value > 126) ? '.' : value;
            ss << value << " ";

            ++current;
            ++page_size;
        }
        ImGui::TextUnformatted(ss.str().c_str());
        ss.str("");
    }

    ImGui::Columns(1);
}
#endif

} // namespace editor