#include "widget/overlay_camera_tracker.h"
#include "imgui/font_awesome.h"
#include "level/scene.h"
#include "erwin.h"
#include "imgui.h"
#include "imgui/imgui_utils.h"

using namespace erwin;

namespace editor
{

CameraTrackerOverlay::CameraTrackerOverlay():
Widget("Camera tracker", true)
{
    flags_ = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar 
           | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize 
           | ImGuiWindowFlags_NoSavedSettings
           | ImGuiWindowFlags_NoNav;
}

void CameraTrackerOverlay::on_imgui_render()
{
    const glm::vec3& cam_pos = Scene::camera_controller.get_camera().get_position();
    const glm::vec3& cam_ang = Scene::camera_controller.get_camera().get_angles();

    ImGui::Text("%s Camera", W_ICON(VIDEO_CAMERA));

    ImGui::Text("x      %.3f", double(cam_pos.x));
    ImGui::Text("y      %.3f", double(cam_pos.y));
    ImGui::Text("z      %.3f", double(cam_pos.z));
    ImGui::Separator();
    ImGui::Text("yaw    %.3f", double(cam_ang.x));
    ImGui::Text("pitch  %.3f", double(cam_ang.y));
    ImGui::Text("roll   %.3f", double(cam_ang.z));
    ImGui::Separator();
    ImGui::Text("aspect %.3f", double(Scene::camera_controller.get_aspect_ratio()));
    ImGui::Text("fovy   %.3f", double(Scene::camera_controller.get_fovy()));
    ImGui::Text("znear  %.3f", double(Scene::camera_controller.get_znear()));
    ImGui::Text("zfar   %.3f", double(Scene::camera_controller.get_zfar()));
}


} // namespace editor