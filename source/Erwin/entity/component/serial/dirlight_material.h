#include "entity/component/dirlight_material.h"
#include "entity/reflection.h"

namespace erwin
{

template <>
void serialize_xml<ComponentDirectionalLightMaterial>(const ComponentDirectionalLightMaterial& cmp, xml::XMLFile& file,
                                                      rapidxml::xml_node<>* cmp_node);

template <>
void deserialize_xml<ComponentDirectionalLightMaterial>(rapidxml::xml_node<>* cmp_node, entt::registry& registry,
                                                        EntityID e);

} // namespace erwin