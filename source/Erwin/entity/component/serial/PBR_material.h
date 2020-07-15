#include "entity/component/PBR_material.h"
#include "entity/reflection.h"

namespace erwin
{
    
template <>
void serialize_xml<ComponentPBRMaterial>(const ComponentPBRMaterial& cmp, xml::XMLFile& file,
                                         rapidxml::xml_node<>* cmp_node);

template <>
void deserialize_xml<ComponentPBRMaterial>(rapidxml::xml_node<>* cmp_node, entt::registry& registry, EntityID e);

} // namespace erwin