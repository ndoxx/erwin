#include "scene.h"
#include "entity/reflection.h"
#include "entity/component/description.h"
#include "render/renderer_3d.h"
#include "render/renderer.h"
#include "render/common_geometry.h"
#include "debug/logger.h"

#include "entity/component/PBR_material.h"
#include "entity/component/bounding_box.h"
#include "entity/component/dirlight_material.h"
#include "entity/component/mesh.h"
#include "entity/component/transform.h"
#include "entity/component/camera.h"
#include "entity/component/light.h"

using namespace erwin;

namespace rtest
{

bool Scene::on_load()
{
    // * Create entities with no async dependency
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
    std::vector<hash_t> future_materials =
    {
        AssetManager::load_material_async(wfs::get_asset_dir() / "materials/greasyMetal.tom"),
        AssetManager::load_material_async(wfs::get_asset_dir() / "materials/scuffedPlastic.tom"),
        AssetManager::load_material_async(wfs::get_asset_dir() / "materials/paintPeelingConcrete.tom"),
        AssetManager::load_material_async(wfs::get_asset_dir() / "materials/dirtyWickerWeave.tom"),
    };
    hash_t future_hdr_tex = AssetManager::load_texture_async(wfs::get_asset_dir() / "hdr/small_cathedral_2k.hdr");

    // * Declare entities dependencies on future resources during entity creation
    for(size_t ii=0; ii<future_materials.size(); ++ii)
    {
        for(size_t jj=0; jj<2; ++jj)
        {
            EntityID ent = create_entity();

            ComponentTransform3D ctransform = {{-4.f + ii*2.f, -1.f + jj*2.f, 0.f}, {0.f, 0.f, 0.f}, 1.5f};
            ComponentMesh cmesh;
            cmesh.set_vertex_array(CommonGeometry::get_vertex_array("cube_pbr"_h));

            registry.assign<ComponentTransform3D>(ent, ctransform);
            registry.assign<ComponentMesh>(ent, cmesh);

            AssetManager::on_material_ready(future_materials[ii], [this, ent=ent](const ComponentPBRMaterial& mat)
            {
                registry.assign<ComponentPBRMaterial>(ent, mat);
            });
        }
    }
    {
        AssetManager::on_texture_ready(future_hdr_tex, [this](const auto& res)
        {
            TextureHandle handle = res.first;
            const Texture2DDescriptor& desc = res.second;
            environment.environment_map = Renderer3D::generate_cubemap_hdr(handle, desc.height);
            Renderer::destroy(handle);
            environment.diffuse_irradiance_map = Renderer3D::generate_irradiance_map(environment.environment_map);
            environment.prefiltered_env_map = Renderer3D::generate_prefiltered_map(environment.environment_map, desc.height);
            Renderer3D::set_environment(environment.diffuse_irradiance_map, environment.prefiltered_env_map);
            Renderer3D::enable_IBL(true);
        });
    }


    // * Launch async loading operations
    AssetManager::launch_async_tasks();

	return true;
}

void Scene::on_unload()
{
    camera = k_invalid_entity_id;
    directional_light = k_invalid_entity_id;
    Renderer::destroy(environment.environment_map);
    Renderer::destroy(environment.diffuse_irradiance_map);
    Renderer::destroy(environment.prefiltered_env_map);
    environment.environment_map = {};
    environment.diffuse_irradiance_map = {};
    environment.prefiltered_env_map = {};
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