#include "entity/component_transform.h"
#include "imgui.h"
#include "imgui/imgui_utils.h"

namespace erwin
{

COMPONENT_DEFINITION(ComponentTransform2D);
bool ComponentTransform2D::init(void* description)
{

	return true;
}
void ComponentTransform2D::inspector_GUI()
{

}

COMPONENT_DEFINITION(ComponentTransform3D);
bool ComponentTransform3D::init(void* description)
{

	return true;
}
void ComponentTransform3D::inspector_GUI()
{
	static constexpr float k_step = 0.1f;
	static constexpr float k_step_fast = 0.5f;

	ImGui::InputFloat("X##cmp_tr3", &position.x, k_step, k_step_fast, "%.3f");
	ImGui::InputFloat("Y##cmp_tr3", &position.y, k_step, k_step_fast, "%.3f");
	ImGui::InputFloat("Z##cmp_tr3", &position.z, k_step, k_step_fast, "%.3f");

	ImGui::SliderFloatDefault("Scale##cmp_tr3", &uniform_scale, 0.1f, 10.0f, 1.f);

	bool update_rotation = false;
    update_rotation |= ImGui::SliderFloatDefault("Pitch##cmp_tr3", &euler.x, -90.f, 90.f, 0.f);
    update_rotation |= ImGui::SliderFloatDefault("Yaw##cmp_tr3",   &euler.y, -180.f, 180.f, 0.f);
    update_rotation |= ImGui::SliderFloatDefault("Roll##cmp_tr3",  &euler.z, -180.f, 180.f, 0.f);
    
    if(update_rotation)
    	set_rotation(euler);
}

} // namespace erwin