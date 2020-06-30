#include "entity/reflection.h"
#include "entity/component/transform.h"

namespace erwin
{
    template <> 
    void serialize_xml<ComponentTransform3D>(const ComponentTransform3D& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
    {
    	file.add_node(cmp_node, "T", to_string(cmp.local.position).c_str());
    	file.add_node(cmp_node, "R", to_string(cmp.local.euler).c_str());
    	file.add_node(cmp_node, "S", std::to_string(cmp.local.uniform_scale).c_str());
    }
}