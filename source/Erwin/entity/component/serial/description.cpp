#include "entity/component/serial/description.h"
#include "imgui/font_awesome.h"
#include "level/scene.h"

namespace erwin
{

template <>
void serialize_xml<ComponentDescription>(const ComponentDescription& cmp, xml::XMLFile& file,
                                         rapidxml::xml_node<>* cmp_node)
{
    file.add_node(cmp_node, "name", cmp.name.c_str());
    file.add_node(cmp_node, "desc", cmp.description.c_str());
}

template <>
void deserialize_xml<ComponentDescription>(rapidxml::xml_node<>* cmp_node, Scene& scene, EntityID e)
{
    std::string name;
    std::string desc;
    xml::parse_node(cmp_node, "name", name);
    xml::parse_node(cmp_node, "desc", desc);

    ComponentDescription cdesc = {name, W_ICON(CUBE), desc};
    scene.add_component<ComponentDescription>(e, cdesc);
}

} // namespace erwin