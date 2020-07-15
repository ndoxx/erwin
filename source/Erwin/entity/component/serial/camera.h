#include "entity/component/camera.h"
#include "entity/reflection.h"

namespace erwin
{
template <>
void serialize_xml<ComponentCamera3D>(const ComponentCamera3D& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node);

template <>
void deserialize_xml<ComponentCamera3D>(rapidxml::xml_node<>* cmp_node, entt::registry& registry, EntityID e);
} // namespace erwin