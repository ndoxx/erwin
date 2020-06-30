#include "entity/reflection.h"
#include "entity/component/dirlight_material.h"

namespace erwin
{
    template <> 
    void serialize_xml<ComponentDirectionalLightMaterial>(const ComponentDirectionalLightMaterial& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
    {
	    file.add_node(cmp_node, "color", to_string(cmp.material_data.color).c_str());
	    file.add_node(cmp_node, "scale", std::to_string(cmp.material_data.scale).c_str());
	    file.add_node(cmp_node, "bright", std::to_string(cmp.material_data.brightness).c_str());
    }
}