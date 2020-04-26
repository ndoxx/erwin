#include "widget/widget_keybindings.h"
#include "input/input.h"
#include "event/event_bus.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

KeybindingsWidget::KeybindingsWidget():
Widget("Key bindings", false)
{
    flags_ = ImGuiWindowFlags_NoDocking;
    selection_ = 0;

    EVENTBUS.subscribe(this, &KeybindingsWidget::on_keyboard_event);
}

bool KeybindingsWidget::on_keyboard_event(const erwin::KeyboardEvent& event)
{
    if(selection_ == 0)
        return false;

    keymap::WKEY new_key = event.key;
    if(new_key != keymap::WKEY::ESCAPE)
        Input::modify_action(selection_, new_key);

    selection_ = 0;

    return true;
}

void KeybindingsWidget::on_imgui_render()
{
    static const char* s_hint = "<press a key>";

    uint32_t action_count = uint32_t(Input::get_action_count());

    ImGui::Columns(2, "Keys");  // 3-ways, no border
    for(uint32_t ii=1; ii<action_count; ++ii)
    {
        const auto& action = Input::get_action(ii);

        bool is_selected = (selection_ == ii);

        ImGui::TextUnformatted(action.description.c_str());
        ImGui::NextColumn();
        if(ImGui::Selectable(is_selected ? s_hint : keymap::KEY_NAMES.at(action.key).c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick))
        {
            if(ImGui::IsMouseDoubleClicked(0))
            {
                selection_ = ii;
            }
        }
        if(is_selected)
            ImGui::SetItemDefaultFocus();
        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::Separator();
    if(ImGui::Button("Apply"))
    {
        Input::save_config();
        open_ = false;
    }
    ImGui::SameLine();
    if(ImGui::Button("Discard"))
    {
        selection_ = 0;
        open_ = false;
    }
}


} // namespace editor