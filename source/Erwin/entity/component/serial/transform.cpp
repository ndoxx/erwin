#include "entity/component/serial/transform.h"
#include "glm/gtx/string_cast.hpp"

namespace erwin
{

template <>
void serialize_xml<ComponentTransform3D>(const ComponentTransform3D& cmp, xml::XMLFile& file,
                                         rapidxml::xml_node<>* cmp_node)
{
    file.add_node(cmp_node, "T", erwin::to_string(cmp.local.position).c_str());
    file.add_node(cmp_node, "R", erwin::to_string(cmp.local.euler).c_str());
    file.add_node(cmp_node, "S", std::to_string(cmp.local.uniform_scale).c_str());
}

template <>
void deserialize_xml<ComponentTransform3D>(rapidxml::xml_node<>* cmp_node, entt::registry& registry, EntityID e)
{
    glm::vec3 T, R;
    float S;
    xml::parse_node(cmp_node, "T", T);
    xml::parse_node(cmp_node, "R", R);
    xml::parse_node(cmp_node, "S", S);
    registry.emplace<ComponentTransform3D>(e, T, R, S);
}

} // namespace erwin