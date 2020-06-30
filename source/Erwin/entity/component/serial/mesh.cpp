#include "entity/reflection.h"
#include "entity/component/mesh.h"

namespace erwin
{
    template <> 
    void serialize_xml<ComponentMesh>(const ComponentMesh& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
    {
    	if(cmp.mesh.procedural)
    		file.add_attribute(cmp_node, "proc", "true");
    	file.add_attribute(cmp_node, "id", to_string(cmp.mesh.resource_id).c_str());
    }
}