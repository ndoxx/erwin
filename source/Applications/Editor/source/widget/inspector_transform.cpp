#include "entity/component/transform.h"
#include "entity/component/tags.h"
#include "entity/reflection.h"
#include "imgui.h"
#include "imgui/imgui_utils.h"

namespace erwin
{

template <> 
void inspector_GUI<ComponentTransform3D>(ComponentTransform3D& cmp, EntityID e, entt::registry& registry, size_t)
{
	static constexpr float k_step = 0.1f;
	static constexpr float k_step_fast = 0.5f;

	bool update_position = false;
	update_position |= ImGui::InputFloat("X##cmp_tr3", &cmp.local.position.x, k_step, k_step_fast, "%.3f");
	update_position |= ImGui::InputFloat("Y##cmp_tr3", &cmp.local.position.y, k_step, k_step_fast, "%.3f");
	update_position |= ImGui::InputFloat("Z##cmp_tr3", &cmp.local.position.z, k_step, k_step_fast, "%.3f");

	bool update_scale = ImGui::SliderFloatDefault("Scale##cmp_tr3", &cmp.local.uniform_scale, 0.1f, 10.0f, 1.f);

	bool update_rotation = false;
    update_rotation |= ImGui::SliderFloatDefault("Pitch##cmp_tr3", &cmp.local.euler.x, -90.f, 90.f, 0.f);
    update_rotation |= ImGui::SliderFloatDefault("Yaw##cmp_tr3",   &cmp.local.euler.y, -180.f, 180.f, 0.f);
    update_rotation |= ImGui::SliderFloatDefault("Roll##cmp_tr3",  &cmp.local.euler.z, -180.f, 180.f, 0.f);
    
    if(update_rotation)
    	cmp.local.set_rotation(cmp.local.euler);

    if(update_position | update_rotation | update_scale)
    	registry.emplace_or_replace<DirtyTransformTag>(e);
}

} // namespace erwin