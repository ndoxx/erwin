#include "entity/component/light.h"
#include "imgui.h"

namespace erwin
{

static float s_inclination_deg   = 90.0f;
static float s_arg_periapsis_deg = 160.0f;

template <> 
void inspector_GUI<ComponentDirectionalLight>(ComponentDirectionalLight* cmp, EntityID, entt::registry&)
{
    if(ImGui::SliderFloat("Inclination", &s_inclination_deg, 0.0f, 180.0f))
        cmp->set_position(s_inclination_deg, s_arg_periapsis_deg);
    if(ImGui::SliderFloat("Arg. periapsis", &s_arg_periapsis_deg, 0.0f, 360.0f))
        cmp->set_position(s_inclination_deg, s_arg_periapsis_deg);

    ImGui::SliderFloat("Brightness", &cmp->brightness, 0.0f, 30.0f);
    ImGui::SliderFloat("Ambient str.", &cmp->ambient_strength, 0.0f, 1.f);
    ImGui::ColorEdit3("Color", &cmp->color[0]);
    ImGui::ColorEdit3("Amb. color", &cmp->ambient_color[0]);
}


} // namespace erwin