#include "entity/component/script.h"
#include "entity/reflection.h"

namespace erwin
{
template <>
void serialize_xml<ComponentScript>(const ComponentScript& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node);

template <>
void deserialize_xml<ComponentScript>(rapidxml::xml_node<>* cmp_node, Scene& scene, EntityID e);
} // namespace erwin