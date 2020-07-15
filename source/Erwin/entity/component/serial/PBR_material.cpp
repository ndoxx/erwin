#include "entity/component/serial/PBR_material.h"

namespace erwin
{
template <>
void serialize_xml<ComponentPBRMaterial>(const ComponentPBRMaterial& cmp, xml::XMLFile& file,
                                         rapidxml::xml_node<>* cmp_node)
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

template <>
void deserialize_xml<ComponentPBRMaterial>(rapidxml::xml_node<>* cmp_node, entt::registry& registry, EntityID e)
{
    size_t resource_id = 0;
    bool override = false;
    xml::parse_attribute(cmp_node, "id", resource_id);
	xml::parse_attribute(cmp_node, "override", override);
	ComponentPBRMaterial::MaterialData md;
	if(override)
	{
        xml::parse_node(cmp_node, "tint", md.tint);
        xml::parse_node(cmp_node, "flags", md.flags);
        xml::parse_node(cmp_node, "emissivity", md.emissive_scale);
        xml::parse_node(cmp_node, "tiling", md.tiling_factor);
        xml::parse_node(cmp_node, "parallax", md.parallax_height_scale);
        xml::parse_node(cmp_node, "u_albedo", md.uniform_albedo);
        xml::parse_node(cmp_node, "u_metal", md.uniform_metallic);
        xml::parse_node(cmp_node, "u_rough", md.uniform_roughness);
	}

    AssetManager::on_ready<ComponentPBRMaterial>(resource_id,
                                                 [&registry, e = e, md = md, override = override](const ComponentPBRMaterial& mat)
	{
		auto& cmp_mat = registry.emplace<ComponentPBRMaterial>(e, mat);
	    if(override)
	    {
			cmp_mat.material_data = md;
			cmp_mat.override = true;
	    }
	});


}

} // namespace erwin