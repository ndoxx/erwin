#include "entity/component/light.h"
#include "entity/reflection.h"

namespace erwin
{

template <>
void serialize_xml<ComponentDirectionalLight>(const ComponentDirectionalLight& cmp, xml::XMLFile& file,
                                              rapidxml::xml_node<>* cmp_node);

template <>
void deserialize_xml<ComponentDirectionalLight>(rapidxml::xml_node<>* cmp_node, entt::registry& registry, EntityID e);

} // namespace erwin