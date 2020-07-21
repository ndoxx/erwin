#include "entity/component/serial/script.h"
#include "script/script_engine.h"
#include "level/scene.h"

namespace erwin
{

template <>
void serialize_xml<ComponentScript>(const ComponentScript& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
{
    file.add_node(cmp_node, "path", cmp.file_path.universal().c_str());
}

template <> void deserialize_xml<ComponentScript>(rapidxml::xml_node<>* cmp_node, Scene& scene, EntityID e)
{
	std::string universal_path;
    xml::parse_node(cmp_node, "path", universal_path);
    
    auto ctx_handle = scene.get_script_context();
    auto& ctx = ScriptEngine::get_context(ctx_handle);
    auto& cscript = scene.add_component<ComponentScript>(e, universal_path);
    ctx.use(cscript.file_path);
}

} // namespace erwin