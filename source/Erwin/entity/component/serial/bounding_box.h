#include "entity/component/bounding_box.h"
#include "entity/reflection.h"

namespace erwin
{

template <> void deserialize_xml<ComponentOBB>(rapidxml::xml_node<>*, entt::registry& registry, EntityID e);

} // namespace erwin