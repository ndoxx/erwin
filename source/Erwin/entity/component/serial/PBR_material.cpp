#include "entity/reflection.h"
#include "entity/component/PBR_material.h"

namespace erwin
{
    template <> 
    void serialize_xml<ComponentPBRMaterial>(const ComponentPBRMaterial& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
    {
    	if(cmp.override)
    	{
    		file.add_attribute(cmp_node, "override", "true");
	    	file.add_node(cmp_node, "tint", to_string(cmp.material_data.tint).c_str());
	    	file.add_node(cmp_node, "flags", std::to_string(cmp.material_data.flags).c_str());
	    	file.add_node(cmp_node, "emissivity", std::to_string(cmp.material_data.emissive_scale).c_str());
	    	file.add_node(cmp_node, "tiling", std::to_string(cmp.material_data.tiling_factor).c_str());
	    	file.add_node(cmp_node, "parallax", std::to_string(cmp.material_data.parallax_height_scale).c_str());
	    	file.add_node(cmp_node, "u_albedo", to_string(cmp.material_data.uniform_albedo).c_str());
	    	file.add_node(cmp_node, "u_metal", std::to_string(cmp.material_data.uniform_metallic).c_str());
	    	file.add_node(cmp_node, "u_rough", std::to_string(cmp.material_data.uniform_roughness).c_str());
	    }

    	file.add_attribute(cmp_node, "id", to_string(cmp.material.resource_id).c_str());
    	// file.add_node(cmp_node, "name", cmp.name.c_str()); // Name unused atm
    }
}