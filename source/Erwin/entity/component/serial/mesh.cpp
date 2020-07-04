#include "entity/component/mesh.h"
#include "asset/asset_manager.h"
#include "entity/reflection.h"
#include "render/common_geometry.h"

namespace erwin
{

template <>
void serialize_xml<ComponentMesh>(const ComponentMesh& cmp, xml::XMLFile& file, rapidxml::xml_node<>* cmp_node)
{
    if(cmp.mesh.procedural)
        file.add_attribute(cmp_node, "proc", "true");
    file.add_attribute(cmp_node, "id", to_string(cmp.mesh.resource_id).c_str());
}

template <> void deserialize_xml<ComponentMesh>(rapidxml::xml_node<>* cmp_node, entt::registry& registry, EntityID e)
{
    bool procedural = false;
    size_t resource_id = 0;
    xml::parse_attribute(cmp_node, "proc", procedural);
    xml::parse_attribute(cmp_node, "id", resource_id);

    if(procedural)
    {
        auto& cmesh = registry.emplace<ComponentMesh>(e);
        cmesh.mesh = CommonGeometry::get_mesh(resource_id);
    }
    else
    {
        AssetManager::on_ready<Mesh>(
            resource_id, [&registry, e = e](const Mesh& mesh) { registry.emplace<ComponentMesh>(e, mesh); });
    }
}

} // namespace erwin