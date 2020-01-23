#include "entity/component_transform.h"
#include "imgui.h"

namespace erwin
{

COMPONENT_DEFINITION(ComponentTransform2D);
bool ComponentTransform2D::init(void* description)
{

	return true;
}
void ComponentTransform2D::inspector_GUI()
{
    ImGui::TextColored({0.f,0.75f,1.f,1.f}, "ComponentTransform2D");
}

COMPONENT_DEFINITION(ComponentTransform3D);
bool ComponentTransform3D::init(void* description)
{

	return true;
}
void ComponentTransform3D::inspector_GUI()
{
    ImGui::TextColored({0.f,0.75f,1.f,1.f}, "ComponentTransform3D");

	static constexpr float k_step = 0.1f;
	static constexpr float k_step_fast = 0.5f;

	ImGui::InputFloat("X##cmp_tr3", &position.x, k_step, k_step_fast, "%.3f");
	ImGui::InputFloat("Y##cmp_tr3", &position.y, k_step, k_step_fast, "%.3f");
	ImGui::InputFloat("Z##cmp_tr3", &position.z, k_step, k_step_fast, "%.3f");

	bool update_rotation = false;
    update_rotation |= ImGui::SliderFloat("Pitch##cmp_tr3", &euler.x, 0.0f, 180.0f);
    update_rotation |= ImGui::SliderFloat("Yaw##cmp_tr3",   &euler.y, 0.0f, 360.0f);
    update_rotation |= ImGui::SliderFloat("Roll##cmp_tr3",  &euler.z, 0.0f, 360.0f);
    
    if(update_rotation)
    	set_rotation(euler);
}

} // namespace erwin