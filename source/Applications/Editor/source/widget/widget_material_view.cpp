#include "widget/widget_material_view.h"
#include "entity/component/PBR_material.h"
#include "entity/component/camera.h"
#include "entity/component/light.h"
#include "entity/component/mesh.h"
#include "entity/component/transform.h"
#include "event/event_bus.h"
#include "event/window_events.h"
#include "render/common_geometry.h"
#include "render/framebuffer_pool.h"
#include "render/handles.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"
#include "level/scene_manager.h"

#include "imgui.h"

using namespace erwin;

namespace editor
{

static constexpr float k_border = 4.f;
static constexpr float k_start_x = 4.f;
static constexpr float k_start_y = 50.f;

MaterialViewWidget::MaterialViewWidget()
    : Widget("Material view", true), render_surface_{0.f, 0.f, 0.f, 0.f, 0.f, 0.f}, current_index_(0)
{
    // flags_ |= ImGuiWindowFlags_MenuBar;
    // Create scene with a sphere in the middle
    auto& scene = SceneManager::create_scene("material_editor_scene"_h);
    scene.load();

    // Create camera
    auto e_cam = scene.registry.create();
    const auto& ctrans = scene.registry.emplace<ComponentTransform3D>(e_cam, glm::vec3(0.f), glm::vec3(0.f), 1.f);
    scene.registry.emplace<ComponentCamera3D>(e_cam);
    scene.set_named(e_cam, "Camera"_h);

    camera_controller_.init(ctrans);
    camera_controller_.set_frustum_parameters({1280.f / 1024.f, 60.f, 0.1f, 100.f});
    camera_controller_.set_position(5.f, 0.f, 90.f); // radius, azimuth, colatitude

    // Create light
    auto e_light = scene.registry.create();
    auto& dirlight = scene.registry.emplace<ComponentDirectionalLight>(e_light);
    dirlight.set_position(47.626f, 49.027f);
    dirlight.color = {0.95f, 0.85f, 0.5f};
    dirlight.ambient_color = {0.95f, 0.85f, 0.5f};
    dirlight.ambient_strength = 0.1f;
    dirlight.brightness = 3.7f;
    scene.set_named(e_cam, "Light"_h);

    // Create renderable object
    auto e_obj = scene.registry.create();
    scene.registry.emplace<ComponentPBRMaterial>(e_obj);
    scene.registry.emplace<ComponentTransform3D>(e_obj, glm::vec3(0.f), glm::vec3(0.f), 1.f);
    scene.registry.emplace<ComponentMesh>(e_obj, CommonGeometry::get_mesh("icosphere_pbr"_h));
    scene.set_named(e_obj, "Object"_h);
}

MaterialViewWidget::~MaterialViewWidget() {}

void MaterialViewWidget::on_update(const GameClock& clock)
{
    auto& scene = SceneManager::get("material_editor_scene"_h);

    scene.registry.view<ComponentTransform3D, ComponentCamera3D>().each([this, &clock](auto, auto& trans, auto& cam) {
        camera_controller_.update(clock, cam, trans);
        Renderer3D::update_camera(cam, trans.local);
    });

    scene.registry.view<ComponentDirectionalLight>().each(
        [](auto, const auto& dirlight) { Renderer3D::update_light(dirlight); });

    Renderer3D::update_frame_data();
}

void MaterialViewWidget::on_resize(uint32_t width, uint32_t height)
{
    float rw = std::max(float(width) - (k_border + k_start_x), 1.f);
    float rh = std::max(float(height) - (k_border + k_start_y), 1.f);
    render_surface_.x1 = render_surface_.x0 + rw;
    render_surface_.y1 = render_surface_.y0 + rh;
    render_surface_.w = rw;
    render_surface_.h = rh;

    EventBus::enqueue(WindowResizeEvent(int(width), int(height)));
    EventBus::enqueue(FramebufferResizeEvent(int(rw), int(rh)));
}

void MaterialViewWidget::on_move(int32_t x, int32_t y)
{
    float rw = std::max(float(width_) - (k_border + k_start_x), 1.f);
    float rh = std::max(float(height_) - (k_border + k_start_y), 1.f);
    render_surface_.x0 = float(x) + k_start_x;
    render_surface_.y0 = float(y) + k_start_y;
    render_surface_.x1 = render_surface_.x0 + rw;
    render_surface_.y1 = render_surface_.y0 + rh;

    EventBus::enqueue(WindowMovedEvent(x, y));
}

void MaterialViewWidget::on_layer_render()
{
    auto& scene = SceneManager::get("material_editor_scene"_h);
    Renderer3D::begin_deferred_pass();
    scene.registry.view<ComponentTransform3D, ComponentMesh, ComponentPBRMaterial>().each(
        [](auto, const auto& trans, const auto& mesh, const auto& mat) {
            Renderer3D::draw_mesh(mesh.mesh, trans.local.get_model_matrix(), mat.material, &mat.material_data);
        });
    Renderer3D::end_deferred_pass();

    // TODO: Use local envmap?
    if(!scn::current().is_loaded())
        return;
    Renderer3D::draw_skybox(scn::current().get_environment().environment_map);
}

static const std::vector<std::string> s_mesh_names = {
    "Icosphere",
    "Cube",
};
static const std::vector<hash_t> s_va_name = {
    "icosphere_pbr"_h,
    "cube_pbr"_h,
};

void MaterialViewWidget::on_imgui_render()
{
    // * Controls
    if(ImGui::BeginCombo("Mesh", s_mesh_names[current_index_].c_str()))
    {
        for(size_t ii = 0; ii < s_va_name.size(); ++ii)
        {
            bool is_selected = (ii == current_index_);
            if(ImGui::Selectable(s_mesh_names[ii].c_str(), is_selected))
            {
                current_index_ = ii;
                auto& scene = SceneManager::get("material_editor_scene"_h);
                auto e_obj = scene.get_named("Object"_h);
                auto& cmesh = scene.registry.get<ComponentMesh>(e_obj);
                cmesh.mesh = CommonGeometry::get_mesh(s_va_name[ii]);
            }
            if(is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // * Show render in this window
    // Retrieve the native framebuffer texture handle
    FramebufferHandle fb = FramebufferPool::get_framebuffer("host"_h);
    TextureHandle texture = Renderer::get_framebuffer_texture(fb, 0);
    void* framebuffer_texture_native = Renderer::get_native_texture_handle(texture);
    ImGui::GetWindowDrawList()->AddImage(framebuffer_texture_native,
                                         // ImGui::GetCursorScreenPos(),
                                         ImVec2(render_surface_.x0, render_surface_.y0),
                                         ImVec2(render_surface_.x1, render_surface_.y1), ImVec2(0, 1), ImVec2(1, 0));
}

} // namespace editor