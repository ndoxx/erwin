#include "entity/reflection.h"
#include "entity/component/description.h"

namespace erwin
{

template <>
void serialize_xml<ComponentDescription>(const ComponentDescription& cmp, xml::XMLFile& file,
                                         rapidxml::xml_node<>* cmp_node);

template <>
void deserialize_xml<ComponentDescription>(rapidxml::xml_node<>* cmp_node, Scene& scene, EntityID e);

} // namespace erwin