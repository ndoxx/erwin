#include "entity/reflection.h"
#include "entity/component/description.h"

namespace erwin
{
    template <> 
    void serialize_xml<ComponentDescription>(const ComponentDescription& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
    {
    	file.add_node(cmp_node, "name", cmp.name.c_str());
		file.add_node(cmp_node, "desc", cmp.description.c_str());
    }
}