#include "entity/component/bounding_box.h"
#include "entity/reflection.h"

namespace erwin
{

template <> void deserialize_xml<ComponentOBB>(rapidxml::xml_node<>*, Scene& scene, EntityID e);

} // namespace erwin