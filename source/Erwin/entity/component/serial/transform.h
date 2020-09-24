#include "entity/component/transform.h"
#include "entity/reflection.h"

namespace erwin
{

template <>
void serialize_xml<ComponentTransform3D>(const ComponentTransform3D& cmp, xml::XMLFile& file,
                                         rapidxml::xml_node<>* cmp_node);

template <>
void deserialize_xml<ComponentTransform3D>(rapidxml::xml_node<>* cmp_node, Scene& scene, EntityID e);

} // namespace erwin