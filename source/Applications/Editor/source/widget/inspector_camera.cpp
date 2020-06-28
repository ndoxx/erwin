#include "entity/component/camera.h"
#include "entity/reflection.h"
#include "imgui.h"
#include "imgui/imgui_utils.h"

namespace erwin
{

template <> 
void inspector_GUI<ComponentCamera3D>(ComponentCamera3D* cmp, EntityID, entt::registry&)
{
    float fovy = (360.f / float(M_PI)) * std::atan(cmp->frustum.top / cmp->frustum.near);

	ImGui::TextColored({1.f,0.75f,0.f,1.f}, "[Frustum]");
    ImGui::Text("fovy: %f°", double(fovy));
    ImGui::Text("l-r: (%f, %f)", double(cmp->frustum.left), double(cmp->frustum.right));
    ImGui::Text("b-t: (%f, %f)", double(cmp->frustum.bottom), double(cmp->frustum.top));
    ImGui::Text("n-f: (%f, %f)", double(cmp->frustum.near), double(cmp->frustum.far));

	ImGui::TextColored({1.f,0.75f,0.f,1.f}, "[Axes]");
    ImGui::Text("r: (%f, %f, %f)", double(cmp->right.x), double(cmp->right.y), double(cmp->right.z));
    ImGui::Text("u: (%f, %f, %f)", double(cmp->up.x), double(cmp->up.y), double(cmp->up.z));
    ImGui::Text("f: (%f, %f, %f)", double(cmp->forward.x), double(cmp->forward.y), double(cmp->forward.z));
}

} // namespace erwin