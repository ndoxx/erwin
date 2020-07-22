#include "layer/layer_scene_view.h"
#include "imgui/font_awesome.h"
#include "level/scene_manager.h"
#include "project/project.h"
#include "script/script_engine.h"

#include <bitset>
#include <iomanip>
#include <iostream>

using namespace erwin;

namespace editor
{

SceneViewLayer::SceneViewLayer() : Layer("SceneViewLayer") {}

void SceneViewLayer::on_imgui_render() {}

void SceneViewLayer::setup_camera(Scene& scene)
{
    ComponentTransform3D& transform = scene.get_component<ComponentTransform3D>(scene.get_named("Camera"_h));

    camera_controller_.init(transform);
    camera_controller_.update_frustum();
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

    auto& scene = scn::current();
    if(!scene.is_loaded())
        return;

    // TODO: traverse UPDATERS only, some scripts may not have an update function
    if(scene.is_runtime())
    {
        auto& ctx = ScriptEngine::get_context(scene.get_script_context());
        ctx.traverse_actors([dt](auto& actor)
        {
            actor.update(dt);
        });
    }

    transform_system_.update(clock, scene);

    auto e_camera = scene.get_named("Camera"_h);
    ComponentCamera3D& camera = scene.get_component<ComponentCamera3D>(e_camera);
    ComponentTransform3D& transform = scene.get_component<ComponentTransform3D>(e_camera);
    camera_controller_.update(clock, camera, transform);
    Renderer3D::update_camera(camera, transform.global);
    if(scene.has_named("Sun"_h))
    {
        auto e_dirlight = scene.get_named("Sun"_h);
        Renderer3D::update_light(scene.get_component<ComponentDirectionalLight>(e_dirlight));
    }
}

void SceneViewLayer::on_render()
{
    auto& scene = scn::current();
    if(!scene.is_loaded())
        return;

    Renderer3D::update_frame_data();
    // Draw scene geometry
    {
        Renderer3D::begin_deferred_pass();
        auto view = scene.view<ComponentTransform3D, ComponentPBRMaterial, ComponentMesh>();
        for(const entt::entity e : view)
        {
            const ComponentTransform3D& ctransform = view.get<ComponentTransform3D>(e);
            const ComponentPBRMaterial& cmaterial = view.get<ComponentPBRMaterial>(e);
            const ComponentMesh& cmesh = view.get<ComponentMesh>(e);
            Renderer3D::draw_mesh(cmesh.mesh, ctransform.global.get_model_matrix(), cmaterial.material,
                                  &cmaterial.material_data);
        }
        Renderer3D::end_deferred_pass();
    }

    Renderer3D::draw_skybox(scene.get_environment().environment_map);

    {
        Renderer3D::begin_forward_pass(BlendState::Light);
        auto view = scene.view<ComponentDirectionalLight, ComponentDirectionalLightMaterial>();
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

    if(Input::match_action(ACTION_FREEFLY_TOGGLE, event))
    {
        freefly_mode_ = !freefly_mode_;
        camera_controller_.transfer_control(freefly_mode_);
        // Camera controller's internal state may have changed if runtime mode
        // was used, restore camera to avoid a jump
        if(freefly_mode_)
            setup_camera(scn::current());
        return true;
    }

    if(!freefly_mode_ && Input::match_action(ACTION_EDITOR_SAVE_SCENE, event))
    {
        scn::current().save();
        return true;
    }

    return camera_controller_.on_keyboard_event(event);
}

} // namespace editor