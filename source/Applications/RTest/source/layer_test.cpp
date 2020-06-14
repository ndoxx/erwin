#include "layer_test.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"
#include "render/renderer.h"
#include "utils/future.hpp"
#include "scene.h"


#include "entity/component_PBR_material.h"
#include "entity/component_bounding_box.h"
#include "entity/component_dirlight_material.h"
#include "entity/component_mesh.h"
#include "entity/component_transform.h"
#include "entity/component_camera.h"
#include "entity/light.h"

using namespace erwin;
using namespace rtest;

LayerTest::LayerTest(): Layer("TestLayer")
{

}

void LayerTest::on_imgui_render()
{
}

void LayerTest::setup_camera()
{
    auto& scene = scn::current<Scene>();

    ComponentTransform3D& transform = scene.registry.get<ComponentTransform3D>(scene.camera);

    camera_controller_.init(transform);
    camera_controller_.set_frustum_parameters({1280.f / 1024.f, 60, 0.1f, 100.f});
}

void LayerTest::on_attach()
{
    wfs::set_asset_dir("source/Applications/RTest/assets");
    bkg_color_ = {0.f,0.f,0.f,1.f};

    SceneManager::create_scene<Scene>("main_scene"_h);
    SceneManager::load_scene("main_scene"_h);
    SceneManager::make_current("main_scene"_h);

    setup_camera();
    Renderer3D::enable_IBL(false);
}

void LayerTest::on_commit()
{
    add_listener(this, &LayerTest::on_mouse_button_event);
    add_listener(this, &LayerTest::on_mouse_moved_event);
    add_listener(this, &LayerTest::on_window_resize_event);
    add_listener(this, &LayerTest::on_window_moved_event);
    add_listener(this, &LayerTest::on_mouse_scroll_event);
    add_listener(this, &LayerTest::on_keyboard_event);
}

void LayerTest::on_detach()
{

}

void LayerTest::on_update(GameClock& clock)
{
    float dt = clock.get_frame_duration();
    static float tt = 0.f;
    tt += dt;
    float r = 0.5f*sin(2*M_PI*tt*0.10f + 1.f) + 0.5f;
    float g = 0.5f*sin(2*M_PI*tt*0.11f + 2.f) + 0.5f;
    float b = 0.5f*sin(2*M_PI*tt*0.09f + 4.f) + 0.5f;
    bkg_color_ = {r,g,b,1.f};

    auto& scene = scn::current<Scene>();
    if(!scene.is_loaded())
        return;

    ComponentCamera3D& camera = scene.registry.get<ComponentCamera3D>(scene.camera);
    ComponentTransform3D& transform = scene.registry.get<ComponentTransform3D>(scene.camera);
    camera_controller_.update(clock, camera, transform);
    Renderer3D::update_camera(camera, transform);
    if(scene.registry.valid(scene.directional_light))
        Renderer3D::update_light(scene.registry.get<ComponentDirectionalLight>(scene.directional_light));

    Renderer3D::update_frame_data();
}

void LayerTest::on_render()
{
    // Renderer::clear(0, Renderer::default_render_target(), ClearFlags::CLEAR_COLOR_FLAG, bkg_color_);

    auto& scene = scn::current<Scene>();
    if(!scene.is_loaded())
        return;

    // Draw scene geometry
    {
        Renderer3D::begin_deferred_pass();
        auto view = scene.registry.view<ComponentTransform3D, ComponentPBRMaterial, ComponentMesh>();
        for(const entt::entity e : view)
        {
            const ComponentTransform3D& ctransform = view.get<ComponentTransform3D>(e);
            const ComponentPBRMaterial& cmaterial = view.get<ComponentPBRMaterial>(e);
            const ComponentMesh& cmesh = view.get<ComponentMesh>(e);
            if(cmaterial.is_ready() && cmesh.is_ready())
                Renderer3D::draw_mesh(cmesh.mesh, ctransform.get_model_matrix(), cmaterial.material,
                                      &cmaterial.material_data);
        }
        Renderer3D::end_deferred_pass();
    }

    Renderer3D::draw_skybox(scene.environment.environment_map);

    PostProcessingRenderer::bloom_pass("LBuffer"_h, 1);
    PostProcessingRenderer::combine("LBuffer"_h, 0, true);
}

bool LayerTest::on_mouse_button_event(const erwin::MouseButtonEvent& event)
{
    if(!enabled_)
        return false;
    return camera_controller_.on_mouse_button_event(event);
}
bool LayerTest::on_mouse_moved_event(const erwin::MouseMovedEvent& event)
{
    if(!enabled_)
        return false;
    return camera_controller_.on_mouse_moved_event(event);
}
bool LayerTest::on_window_resize_event(const erwin::WindowResizeEvent& event)
{
    camera_controller_.on_window_resize_event(event);
    return false;
}
bool LayerTest::on_window_moved_event(const erwin::WindowMovedEvent& event)
{
    if(!enabled_)
        return false;
    return camera_controller_.on_window_moved_event(event);
}
bool LayerTest::on_mouse_scroll_event(const erwin::MouseScrollEvent& event)
{
    if(!enabled_)
        return false;
    return camera_controller_.on_mouse_scroll_event(event);
}
bool LayerTest::on_keyboard_event(const erwin::KeyboardEvent& event)
{
    if(!enabled_)
        return false;
    return camera_controller_.on_keyboard_event(event);
}
