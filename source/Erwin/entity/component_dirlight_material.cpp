#include "entity/component_dirlight_material.h"
#include "imgui.h"

namespace erwin
{

template <>
void inspector_GUI<ComponentDirectionalLightMaterial>(ComponentDirectionalLightMaterial* cmp)
{
    ImGui::SliderFloat("App. diameter", &cmp->material_data.scale, 0.1f, 0.4f);
}

} // namespace erwin