#include "entity/reflection.h"
#include "entity/component/description.h"

namespace erwin
{
    template <> 
    void serialize_xml<ComponentDescription>(const ComponentDescription& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
    {
    	file.add_attribute(cmp_node, "name", cmp.name.c_str());
    	file.set_value(cmp_node, cmp.description.c_str());
    }
}