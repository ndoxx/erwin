#include "system/gizmo_system.h"
#include "entity/component_camera.h"
#include "entity/component_transform.h"
#include "asset/bounding.h"
#include "level/scene.h"
#include "asset/asset_manager.h"
#include "render/renderer_3d.h"

namespace erwin
{

GizmoSystem::GizmoSystem()
{
    auto gizmo_shader = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/gizmo.glsl", "gizmo");
    auto gizmo_ubo    = Renderer::create_uniform_buffer("gizmo_data", nullptr, sizeof(GizmoData), UsagePattern::Dynamic);
    gizmo_material_   = {"gizmo"_h, {}, gizmo_shader, gizmo_ubo, sizeof(GizmoData)};
    Renderer3D::register_shader(gizmo_shader);
    Renderer::shader_attach_uniform_buffer(gizmo_shader, gizmo_ubo);

    selected_part_ = -1;
}

GizmoSystem::~GizmoSystem()
{

}

bool GizmoSystem::on_ray_scene_query_event(const RaySceneQueryEvent& event)
{
    auto& scene = scn::current<EdScene>();
    if(scene.selected_entity == k_invalid_entity_id)
        return false;

    // Get selected entity's transform if any
    auto* transform = scene.registry.try_get<ComponentTransform3D>(scene.selected_entity);
    if(transform == nullptr)
    	return false;

    glm::mat4 parent_model = transform->get_unscaled_model_matrix();

    const ComponentCamera3D& camera = scene.registry.get<ComponentCamera3D>(scene.camera);
    glm::mat4 VP_inv = glm::inverse(camera.view_projection_matrix);
    Ray ray(event.coords, VP_inv);

	// constexpr float k_cyl_diameter = 0.01f;
	constexpr float k_cyl_length = 2.f;
	constexpr float k_arrow_diameter = 0.1f;
	constexpr float k_arrow_length = 0.3f;
	constexpr float k_offset = k_cyl_length+0.5f*k_arrow_length;
	constexpr float k_OBB_scale = 1.5f;
    static glm::vec3 offsets[] =
    {
    	{0.f,   0.f,    0.f},
    	{k_offset, 0.f, 0.f},
    	{0.f, k_offset, 0.f},
    	{0.f, 0.f, k_offset},
    };
    static Extent OBB_extent
    (
    	-k_OBB_scale * k_arrow_diameter, k_OBB_scale * k_arrow_diameter,
    	-k_OBB_scale * k_arrow_diameter, k_OBB_scale * k_arrow_diameter,
    	-k_OBB_scale * k_arrow_diameter, k_OBB_scale * k_arrow_diameter
    );

    float nearest = camera.frustum.far;
    Ray::CollisionData data;

    selected_part_ = -1;
	for(int ii=0; ii<4; ++ii)
	{
		glm::mat OBB_model = glm::translate(parent_model, offsets[ii]);

        if(ray.collides_OBB(OBB_model, OBB_extent, 1.f, data))
        {
            if(data.near < nearest)
            {
                nearest = data.near;
                selected_part_ = ii;
            }
        }
	}

	return (selected_part_ != -1);
}

void GizmoSystem::render()
{
    auto& scene = scn::current<EdScene>();
    if(scene.selected_entity == k_invalid_entity_id)
        return;

    auto* transform = scene.registry.try_get<ComponentTransform3D>(scene.selected_entity);
    if(transform == nullptr)
        return;

    // Draw gizmo
    gizmo_data_.selected = selected_part_;

    Renderer3D::begin_line_pass(false);
    Renderer3D::draw_mesh(CommonGeometry::get_vertex_array("origin_lines"_h), 
    						   transform->get_unscaled_model_matrix(), 
    						   gizmo_material_,
                               &gizmo_data_);
    Renderer3D::end_line_pass();
}

} // namespace erwin