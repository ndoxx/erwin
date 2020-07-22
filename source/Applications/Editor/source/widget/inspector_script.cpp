#include "entity/component/script.h"
#include "entity/reflection.h"
#include "widget/dialog_open.h"
#include "project/project.h"
#include "imgui.h"

namespace erwin
{

template <> 
void inspector_GUI<ComponentScript>(ComponentScript& cmp, EntityID, entt::registry&, size_t)
{
    // Load material from file
    if(ImGui::Button("Load"))
        editor::dialog::show_open("ChooseScriptDlgKey", "Choose script file", ".chai", editor::project::asset_dir(editor::DK::SCRIPT).absolute());

    editor::dialog::on_open("ChooseScriptDlgKey", [&cmp](const fs::path& filepath)
    {
        cmp = ComponentScript(WPath("res", filepath));
    });

    if(!cmp.file_path.empty())
    {
        ImGui::TextColored({0.1f,0.5f,1.f,1.f}, "%s", cmp.file_path.universal().c_str());
        ImGui::Text("entry: %s", cmp.entry_point.c_str());
    }
}

} // namespace erwin