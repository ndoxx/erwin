#include "entity/component/script.h"
#include "entity/reflection.h"
#include "imgui.h"
#include "imgui/imgui_utils.h"
#include "level/scene.h"
#include "project/project.h"
#include "widget/dialog_open.h"

namespace erwin
{

template <> void inspector_GUI<ComponentScript>(ComponentScript& cmp, EntityID e, Scene& scene)
{
    // Load material from file
    if(!scene.is_runtime())
    {
        if(ImGui::Button("Load"))
            editor::dialog::show_open("ChooseScriptDlgKey", "Choose script file", ".chai",
                                      editor::project::asset_dir(editor::DK::SCRIPT).absolute());

        editor::dialog::on_open("ChooseScriptDlgKey", [&cmp,&scene,e](const fs::path& filepath)
        {
            cmp = ComponentScript(std::string("res", filepath));
            auto ctx_handle = scene.get_script_context();
            auto& ctx = ScriptEngine::get_context(ctx_handle);
            ctx.setup_component(cmp, e);
        });
    }

    if(!cmp.file_path.empty())
    {
        ImGui::TextColored({0.1f, 0.5f, 1.f, 1.f}, "%s", cmp.file_path.universal().c_str());
        ImGui::Text("entry point: %s", cmp.entry_point.c_str());

        // Parameters
        auto& ctx = ScriptEngine::get_context(scene.get_script_context());
        auto& actor = ctx.get_actor(cmp.actor_index);
        const auto& params = ctx.get_reflection(actor.actor_type).parameters;

        ImGui::TextUnformatted("Parameters:");
        for(const auto& param : params)
        {
            switch(param.type)
            {
            case "float"_h: {
                ImGui::SliderFloatDefault(param.name.c_str(), actor.get_parameter_ptr<float>(param.name),
                                          std::get<0>(param.range), std::get<1>(param.range), std::get<2>(param.range));
                break;
            }
            default: {
            }
            }
        }
    }
}

} // namespace erwin