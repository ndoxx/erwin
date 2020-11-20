#include "entity/component/serial/light.h"
#include "entity/reflection.h"
#include "level/scene.h"

namespace erwin
{

template <>
void serialize_xml<ComponentDirectionalLight>(const ComponentDirectionalLight& cmp, xml::XMLFile& file,
                                              rapidxml::xml_node<>* cmp_node)
{
    file.add_node(cmp_node, "inclination", std::to_string(cmp.inclination).c_str());
    file.add_node(cmp_node, "arg_periapsis", std::to_string(cmp.arg_periapsis).c_str());
    file.add_node(cmp_node, "color", kb::to_string(cmp.color).c_str());
    file.add_node(cmp_node, "ambient_color", kb::to_string(cmp.ambient_color).c_str());
    file.add_node(cmp_node, "ambient_strength", kb::to_string(cmp.ambient_strength).c_str());
    file.add_node(cmp_node, "brightness", kb::to_string(cmp.brightness).c_str());
}

template <>
void deserialize_xml<ComponentDirectionalLight>(rapidxml::xml_node<>* cmp_node, Scene& scene, EntityID e)
{
    auto& cmp_dl = scene.add_component<ComponentDirectionalLight>(e);

    float incl = 0.f;
    float ap = 0.f;
    xml::parse_node(cmp_node, "inclination", incl);
    xml::parse_node(cmp_node, "arg_periapsis", ap);
    cmp_dl.set_position(incl, ap);
    xml::parse_node(cmp_node, "color", cmp_dl.color);
    xml::parse_node(cmp_node, "ambient_color", cmp_dl.ambient_color);
    xml::parse_node(cmp_node, "ambient_strength", cmp_dl.ambient_strength);
    xml::parse_node(cmp_node, "brightness", cmp_dl.brightness);
}

} // namespace erwin