#include "entity/component/serial/dirlight_material.h"
#include "asset/asset_manager.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"
#include "level/scene.h"

namespace erwin
{

template <>
void serialize_xml<ComponentDirectionalLightMaterial>(const ComponentDirectionalLightMaterial& cmp, xml::XMLFile& file,
                                                      rapidxml::xml_node<>* cmp_node)
{
    file.add_node(cmp_node, "color", to_string(cmp.material_data.color).c_str());
    file.add_node(cmp_node, "scale", std::to_string(cmp.material_data.scale).c_str());
    file.add_node(cmp_node, "bright", std::to_string(cmp.material_data.brightness).c_str());
}

template <>
void deserialize_xml<ComponentDirectionalLightMaterial>(rapidxml::xml_node<>* cmp_node, Scene& scene, EntityID e)
{
    Material mat_sun;
    mat_sun.archetype = "Sun"_h;
    mat_sun.shader = AssetManager::load_shader("shaders/forward_sun.glsl");
    mat_sun.ubo = AssetManager::create_material_data_buffer<ComponentDirectionalLightMaterial>();
    mat_sun.data_size = sizeof(ComponentDirectionalLightMaterial::MaterialData);
    Renderer3D::register_shader(mat_sun.shader);
    Renderer::shader_attach_uniform_buffer(mat_sun.shader, mat_sun.ubo);

    auto& cmp_dlm = scene.add_component<ComponentDirectionalLightMaterial>(e);
    cmp_dlm.set_material(mat_sun);

    xml::parse_node(cmp_node, "color", cmp_dlm.material_data.color);
    xml::parse_node(cmp_node, "scale", cmp_dlm.material_data.scale);
    xml::parse_node(cmp_node, "bright", cmp_dlm.material_data.brightness);
}

} // namespace erwin