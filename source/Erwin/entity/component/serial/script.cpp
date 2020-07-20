#include "entity/component/serial/script.h"
#include "script/script_engine.h"

namespace erwin
{

template <>
void serialize_xml<ComponentScript>(const ComponentScript& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
{
    (void)cmp;
    (void)file;
    (void)cmp_node;
}

template <> void deserialize_xml<ComponentScript>(rapidxml::xml_node<>* cmp_node, entt::registry& registry, EntityID e)
{
    (void)cmp_node;
    (void)registry;
    (void)e;
}

} // namespace erwin