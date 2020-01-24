#include "overlay_camera_tracker.h"
#include "game/scene.h"
#include "erwin.h"
#include "imgui.h"
#include "imgui/imgui_utils.h"
#include "font_awesome.h"

using namespace erwin;

namespace editor
{

CameraTrackerOverlay::CameraTrackerOverlay(game::Scene& scene):
Widget("Camera tracker", true),
scene_(scene)
{
    flags_ = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar 
           | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize 
           | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing 
           | ImGuiWindowFlags_NoNav;
}

CameraTrackerOverlay::~CameraTrackerOverlay()
{

}

void CameraTrackerOverlay::on_imgui_render()
{
    const glm::vec3& cam_pos = scene_.camera_controller.get_camera().get_position();
    const glm::vec3& cam_ang = scene_.camera_controller.get_camera().get_angles();

    ImGui::Text("%s Camera", ICON_FA_VIDEO_CAMERA);

    ImGui::Text("x      %.3f", cam_pos.x);
    ImGui::Text("y      %.3f", cam_pos.y);
    ImGui::Text("z      %.3f", cam_pos.z);
    ImGui::Separator();
    ImGui::Text("yaw    %.3f", cam_ang.x);
    ImGui::Text("pitch  %.3f", cam_ang.y);
    ImGui::Text("roll   %.3f", cam_ang.z);
    ImGui::Separator();
    ImGui::Text("aspect %.3f", scene_.camera_controller.get_aspect_ratio());
    ImGui::Text("fovy   %.3f", scene_.camera_controller.get_fovy());
    ImGui::Text("znear  %.3f", scene_.camera_controller.get_znear());
    ImGui::Text("zfar   %.3f", scene_.camera_controller.get_zfar());
}


} // namespace editor