#include "entity/light.h"
#include "imgui.h"

namespace erwin
{

template <> 
void inspector_GUI<ComponentDirectionalLight>(ComponentDirectionalLight* cmp)
{
    static float inclination_deg   = 90.0f;
    static float arg_periapsis_deg = 160.0f;

    if(ImGui::SliderFloat("Inclination", &inclination_deg, 0.0f, 180.0f))
        cmp->set_position(inclination_deg, arg_periapsis_deg);
    if(ImGui::SliderFloat("Arg. periapsis", &arg_periapsis_deg, 0.0f, 360.0f))
        cmp->set_position(inclination_deg, arg_periapsis_deg);

    ImGui::SliderFloat("Brightness", &cmp->brightness, 0.0f, 30.0f);
    ImGui::SliderFloat("Ambient str.", &cmp->ambient_strength, 0.0f, 0.5f);
    ImGui::ColorEdit3("Color", (float*)&cmp->color);
    ImGui::ColorEdit3("Amb. color", (float*)&cmp->ambient_color);
}


} // namespace erwin