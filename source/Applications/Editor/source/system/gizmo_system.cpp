#include "system/gizmo_system.h"
#include "entity/component_camera.h"
#include "entity/component_transform.h"
#include "entity/tag_components.h"
#include "asset/bounding.h"
#include "level/scene.h"
#include "render/renderer_3d.h"

using namespace erwin;

namespace editor
{

GizmoSystem::GizmoSystem()
{
    auto gizmo_shader = Renderer::create_shader(wfs::get_system_asset_dir() / "shaders/gizmo.glsl", "gizmo");
    auto gizmo_ubo    = Renderer::create_uniform_buffer("gizmo_data", nullptr, sizeof(GizmoData), UsagePattern::Dynamic);
    gizmo_material_   = {"gizmo"_h, {}, gizmo_shader, gizmo_ubo, sizeof(GizmoData)};
    Renderer3D::register_shader(gizmo_shader);
    Renderer::shader_attach_uniform_buffer(gizmo_shader, gizmo_ubo);

    selected_part_ = -1;
}

GizmoSystem::~GizmoSystem()
{

}

// constexpr float k_cyl_diameter = 0.01f;
constexpr float k_cyl_length = 2.f;
constexpr float k_arrow_diameter = 0.1f;
constexpr float k_arrow_length = 0.3f;
constexpr float k_offset = k_cyl_length+0.5f*k_arrow_length;
constexpr float k_OBB_scale = 1.5f;

constexpr glm::vec3 offsets[] =
{
    {0.f,   0.f,    0.f},
    {k_offset, 0.f, 0.f},
    {0.f, k_offset, 0.f},
    {0.f, 0.f, k_offset},
};
static const Extent OBB_extent
(
    -k_OBB_scale * k_arrow_diameter, k_OBB_scale * k_arrow_diameter,
    -k_OBB_scale * k_arrow_diameter, k_OBB_scale * k_arrow_diameter,
    -k_OBB_scale * k_arrow_diameter, k_OBB_scale * k_arrow_diameter
);

bool GizmoSystem::on_ray_scene_query_event(const RaySceneQueryEvent& event)
{
    auto& scene = scn::current<Scene>();
    if(!scene.is_loaded())
        return false;

    const ComponentCamera3D& camera = scene.registry.get<ComponentCamera3D>(scene.camera);
    float nearest = camera.frustum.far;
    glm::mat4 VP_inv = glm::inverse(camera.view_projection_matrix);
    Ray ray(event.coords, VP_inv);

    bool event_handled = false;
    auto view = scene.registry.view<SelectedTag, ComponentTransform3D>();
    for(const entt::entity e : view)
    {
        const ComponentTransform3D& transform = view.get<ComponentTransform3D>(e);
        glm::mat4 parent_model = transform.get_unscaled_model_matrix();

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
                    event_handled = true;
                }
            }
    	}
    }
    return event_handled;
}

void GizmoSystem::render(const Scene& scene)
{
    auto view = scene.registry.view<const SelectedTag, const ComponentTransform3D>();
    for(const entt::entity e : view)
    {
        // Draw gizmo
        const ComponentTransform3D& transform = view.get<const ComponentTransform3D>(e);
        gizmo_data_.selected = selected_part_;

        Renderer3D::begin_line_pass(false);
        Renderer3D::draw_mesh(CommonGeometry::get_mesh("origin_lines"_h), 
                                   transform.get_unscaled_model_matrix(), 
                                   gizmo_material_,
                                   &gizmo_data_);
        Renderer3D::end_line_pass();
    }

}

} // namespace editor