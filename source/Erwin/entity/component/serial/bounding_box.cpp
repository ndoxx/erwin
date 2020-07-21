#include "entity/component/serial/bounding_box.h"
#include "level/scene.h"

namespace erwin
{

template <> void deserialize_xml<ComponentOBB>(rapidxml::xml_node<>*, Scene& scene, EntityID e)
{
    scene.add_component<ComponentOBB>(e);
}

} // namespace erwin