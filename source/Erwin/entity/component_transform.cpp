#include "entity/component_transform.h"
#include "imgui.h"
#include "imgui/imgui_utils.h"

namespace erwin
{

template <> 
void inspector_GUI<ComponentTransform3D>(void* data)
{
    ComponentTransform3D* cmp = static_cast<ComponentTransform3D*>(data);

	static constexpr float k_step = 0.1f;
	static constexpr float k_step_fast = 0.5f;

	ImGui::InputFloat("X##cmp_tr3", &cmp->position.x, k_step, k_step_fast, "%.3f");
	ImGui::InputFloat("Y##cmp_tr3", &cmp->position.y, k_step, k_step_fast, "%.3f");
	ImGui::InputFloat("Z##cmp_tr3", &cmp->position.z, k_step, k_step_fast, "%.3f");

	ImGui::SliderFloatDefault("Scale##cmp_tr3", &cmp->uniform_scale, 0.1f, 10.0f, 1.f);

	bool update_rotation = false;
    update_rotation |= ImGui::SliderFloatDefault("Pitch##cmp_tr3", &cmp->euler.x, -90.f, 90.f, 0.f);
    update_rotation |= ImGui::SliderFloatDefault("Yaw##cmp_tr3",   &cmp->euler.y, -180.f, 180.f, 0.f);
    update_rotation |= ImGui::SliderFloatDefault("Roll##cmp_tr3",  &cmp->euler.z, -180.f, 180.f, 0.f);
    
    if(update_rotation)
    	cmp->set_rotation(cmp->euler);
}

} // namespace erwin