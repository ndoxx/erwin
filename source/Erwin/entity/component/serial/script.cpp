#include "entity/component/serial/script.h"
#include "level/scene.h"
#include "script/script_engine.h"

namespace erwin
{

template <>
void serialize_xml<ComponentScript>(const ComponentScript& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
{
    file.add_node(cmp_node, "path", cmp.file_path.universal().c_str());

    // Save actor parameters
    auto& ctx = ScriptEngine::get_context(cmp.script_context);
    auto& actor = ctx.get_actor(cmp.actor_index);
    const auto& params = ctx.get_reflection(actor.actor_type).parameters;

    auto* params_node = file.add_node(cmp_node, "parameters");
    for(const auto& param : params)
    {
        switch(param.type)
        {
        case "float"_h: {
            auto* param_node = file.add_node(params_node, "float");
            float value = actor.get_parameter<float>(param.name);
            file.add_attribute(param_node, "name", param.name.c_str());
            file.add_attribute(param_node, "value", std::to_string(value).c_str());
            break;
        }
        default: {
        }
        }
    }
}

static void set_actor_property(script::Actor& actor, rapidxml::xml_node<>* xnode)
{
    std::string str_param_name;
    if(!xml::parse_attribute(xnode, "name", str_param_name))
        return;

    // Get hash from node name
    hash_t type_h = H_(xnode->name());
    switch(type_h)
    {
    case "float"_h: {
        if(actor.has_parameter<float>(str_param_name))
            xml::parse_attribute(xnode, "value", actor.get_parameter<float>(str_param_name).get());
        break;
    }
    default: {
    }
    }
}

template <> void deserialize_xml<ComponentScript>(rapidxml::xml_node<>* cmp_node, Scene& scene, EntityID e)
{
    std::string universal_path;
    xml::parse_node(cmp_node, "path", universal_path);

    WPath script_path(universal_path);
    if(script_path.exists() && !script_path.empty())
    {
        auto& cscript = scene.add_component<ComponentScript>(e, script_path);
        auto ctx_handle = scene.get_script_context();
        auto& ctx = ScriptEngine::get_context(ctx_handle);
        ctx.setup_component(cscript, e);

        // Parse XML for parameter values and init actor instance
        auto& actor = ctx.get_actor(cscript.actor_index);

        auto* params_node = cmp_node->first_node("parameters");
        if(params_node)
            for(rapidxml::xml_node<>* param_node = params_node->first_node(); param_node;
                param_node = param_node->next_sibling())
                set_actor_property(actor, param_node);
    }
}

} // namespace erwin