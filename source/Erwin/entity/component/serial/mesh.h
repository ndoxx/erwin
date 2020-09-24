#include "entity/component/mesh.h"
#include "entity/reflection.h"

namespace erwin
{

template <>
void serialize_xml<ComponentMesh>(const ComponentMesh& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node);

template <> void deserialize_xml<ComponentMesh>(rapidxml::xml_node<>* cmp_node, Scene& scene, EntityID e);

} // namespace erwin