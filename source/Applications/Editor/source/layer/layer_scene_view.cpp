#include "layer_scene_view.h"
#include "imgui/font_awesome.h"
#include "level/scene.h"
#include "project/project.h"

#include <bitset>
#include <iomanip>
#include <iostream>

using namespace erwin;

namespace editor
{

SceneViewLayer::SceneViewLayer() : Layer("SceneViewLayer") {}

void SceneViewLayer::on_imgui_render() {}

void SceneViewLayer::setup_camera()
{
    auto& scene = scn::current<Scene>();

    ComponentTransform3D& transform = scene.registry.get<ComponentTransform3D>(scene.camera);

    camera_controller_.init(transform);
    camera_controller_.set_frustum_parameters({1280.f / 1024.f, 60, 0.1f, 100.f});
}

void SceneViewLayer::on_attach() {}

void SceneViewLayer::on_commit()
{
    add_listener(this, &SceneViewLayer::on_mouse_button_event);
    add_listener(this, &SceneViewLayer::on_mouse_moved_event);
    add_listener(this, &SceneViewLayer::on_window_resize_event);
    add_listener(this, &SceneViewLayer::on_window_moved_event);
    add_listener(this, &SceneViewLayer::on_mouse_scroll_event);
    add_listener(this, &SceneViewLayer::on_keyboard_event);
}

void SceneViewLayer::on_detach() {}

void SceneViewLayer::on_update(GameClock& clock)
{
    float dt = clock.get_frame_duration();
    static float tt = 0.f;
    tt += dt;
    if(tt >= 10.f)
        tt = 0.f;

    auto& scene = scn::current<Scene>();
    if(!scene.is_loaded())
        return;

    ComponentCamera3D& camera = scene.registry.get<ComponentCamera3D>(scene.camera);
    ComponentTransform3D& transform = scene.registry.get<ComponentTransform3D>(scene.camera);
    camera_controller_.update(clock, camera, transform);
    Renderer3D::update_camera(camera, transform.local);
    if(scene.registry.valid(scene.directional_light))
        Renderer3D::update_light(scene.registry.get<ComponentDirectionalLight>(scene.directional_light));
}

void SceneViewLayer::on_render()
{
    auto& scene = scn::current<Scene>();
    if(!scene.is_loaded())
        return;

    Renderer3D::update_frame_data();
    // Draw scene geometry
    {
        Renderer3D::begin_deferred_pass();
        auto view = scene.registry.view<ComponentTransform3D, ComponentPBRMaterial, ComponentMesh>();
        for(const entt::entity e : view)
        {
            const ComponentTransform3D& ctransform = view.get<ComponentTransform3D>(e);
            const ComponentPBRMaterial& cmaterial = view.get<ComponentPBRMaterial>(e);
            const ComponentMesh& cmesh = view.get<ComponentMesh>(e);
            // TMP: use global transform when we have a system to update it
            Renderer3D::draw_mesh(cmesh.mesh, ctransform.local.get_model_matrix(), cmaterial.material,
                                  &cmaterial.material_data);
        }
        Renderer3D::end_deferred_pass();
    }

    Renderer3D::draw_skybox(scene.environment.environment_map);

    {
        Renderer3D::begin_forward_pass(BlendState::Light);
        auto view = scene.registry.view<ComponentDirectionalLight, ComponentDirectionalLightMaterial>();
        for(const entt::entity e : view)
        {
            const ComponentDirectionalLight& dirlight = view.get<ComponentDirectionalLight>(e);
            ComponentDirectionalLightMaterial& renderable = view.get<ComponentDirectionalLightMaterial>(e);
            if(!renderable.is_ready())
                continue;

            renderable.material_data.color = glm::vec4(dirlight.color, 1.f);
            renderable.material_data.brightness = dirlight.brightness;

            Renderer3D::draw_mesh(CommonGeometry::get_mesh("quad"_h), glm::mat4(1.f), renderable.material,
                                  &renderable.material_data);
        }
        Renderer3D::end_forward_pass();
    }
}

bool SceneViewLayer::on_mouse_button_event(const erwin::MouseButtonEvent& event)
{
    if(!enabled_)
        return false;
    return camera_controller_.on_mouse_button_event(event);
}
bool SceneViewLayer::on_mouse_moved_event(const erwin::MouseMovedEvent& event)
{
    if(!enabled_)
        return false;
    return camera_controller_.on_mouse_moved_event(event);
}
bool SceneViewLayer::on_window_resize_event(const erwin::WindowResizeEvent& event)
{
    camera_controller_.on_window_resize_event(event);
    return false;
}
bool SceneViewLayer::on_window_moved_event(const erwin::WindowMovedEvent& event)
{
    if(!enabled_)
        return false;
    return camera_controller_.on_window_moved_event(event);
}
bool SceneViewLayer::on_mouse_scroll_event(const erwin::MouseScrollEvent& event)
{
    if(!enabled_)
        return false;
    return camera_controller_.on_mouse_scroll_event(event);
}
bool SceneViewLayer::on_keyboard_event(const erwin::KeyboardEvent& event)
{
    if(!enabled_)
        return false;
    return camera_controller_.on_keyboard_event(event);
}

} // namespace editor