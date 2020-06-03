#include "scene.h"
#include "entity/reflection.h"
#include "entity/component_description.h"
#include "render/renderer_3d.h"
#include "render/renderer.h"
#include "render/common_geometry.h"
#include "debug/logger.h"

#include "entity/component_PBR_material.h"
#include "entity/component_bounding_box.h"
#include "entity/component_dirlight_material.h"
#include "entity/component_mesh.h"
#include "entity/component_transform.h"
#include "entity/component_camera.h"
#include "entity/light.h"

// TMP
#include "asset/asset_manager.h"

using namespace erwin;

namespace rtest
{

bool Scene::on_load()
{
    {
        EntityID ent = create_entity();

        ComponentTransform3D transform({-5.8f, 2.3f, -5.8f}, {5.f, 228.f, 0.f}, 1.f);

        registry.assign<ComponentCamera3D>(ent);
        registry.assign<ComponentTransform3D>(ent, transform);
        camera = ent;
    }

    {
        Material mat_sun;
        mat_sun.archetype = "Sun"_h;
        mat_sun.shader = AssetManager::load_shader("shaders/forward_sun.glsl");
        mat_sun.ubo = AssetManager::create_material_data_buffer<ComponentDirectionalLightMaterial>();
        mat_sun.data_size = sizeof(ComponentDirectionalLightMaterial::MaterialData);
        Renderer3D::register_shader(mat_sun.shader);
        Renderer::shader_attach_uniform_buffer(mat_sun.shader, mat_sun.ubo);

        EntityID ent = registry.create();

        ComponentDirectionalLight cdirlight;
        cdirlight.set_position(47.626f, 49.027f);
        cdirlight.color = {0.95f, 0.85f, 0.5f};
        cdirlight.ambient_color = {0.95f, 0.85f, 0.5f};
        cdirlight.ambient_strength = 0.3f;
        cdirlight.brightness = 3.7f;

        ComponentDirectionalLightMaterial renderable;
        renderable.set_material(mat_sun);
        renderable.material_data.scale = 0.2f;

        registry.assign<ComponentDirectionalLight>(ent, cdirlight);
        registry.assign<ComponentDirectionalLightMaterial>(ent, renderable);

        directional_light = ent;
        add_entity(ent);
    }


    // * Load all resources asynchronously
    auto future_mat_0 = AssetManagerE::load_material_async(wfs::get_asset_dir() / "materials/greasyMetal.tom");

    // * Declare entities dependencies on future resources
    {
        EntityID ent = create_entity();

        ComponentTransform3D ctransform = {{0.f, 0.f, 0.f}, {0.f, 0.f, 0.f}, 1.8f};
        ComponentMesh cmesh;
        cmesh.set_vertex_array(CommonGeometry::get_vertex_array("cube_pbr"_h));

        registry.assign<ComponentTransform3D>(ent, ctransform);
        registry.assign<ComponentMesh>(ent, cmesh);

        AssetManagerE::on_material_ready(future_mat_0, [this, ent=ent](const ComponentPBRMaterial& mat)
        {
            registry.assign<ComponentPBRMaterial>(ent, mat);
            DLOGW("application") << "Material ready. Adding component." << std::endl;
        });
    }

    // * Launch async loading operations
    AssetManagerE::launch_async_tasks();

	return true;
}

void Scene::on_unload()
{
    camera = k_invalid_entity_id;
    directional_light = k_invalid_entity_id;
    registry.clear();
    entities.clear();
}

void Scene::cleanup()
{

}

void Scene::add_entity(EntityID entity)
{
	entities.push_back(entity);

	DLOG("application",1) << "[Scene] Added entity: " << uint64_t(entity) << std::endl;
}

} // namespace rtest