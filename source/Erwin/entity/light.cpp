#include "entity/light.h"
#include "imgui.h"

namespace erwin
{

COMPONENT_DEFINITION(ComponentDirectionalLight);
bool ComponentDirectionalLight::init(void* description)
{

	return true;
}
void ComponentDirectionalLight::inspector_GUI()
{
	static float inclination_deg   = 90.0f;
	static float arg_periapsis_deg = 160.0f;

    ImGui::TextColored({0.f,0.75f,1.f,1.f}, "ComponentDirectionalLight");

    if(ImGui::SliderFloat("Inclination", &inclination_deg, 0.0f, 180.0f))
    	set_position(inclination_deg, arg_periapsis_deg);
    if(ImGui::SliderFloat("Arg. periapsis", &arg_periapsis_deg, 0.0f, 360.0f))
    	set_position(inclination_deg, arg_periapsis_deg);

    ImGui::SliderFloat("Brightness", &brightness, 0.0f, 30.0f);
    ImGui::SliderFloat("Ambient str.", &ambient_strength, 0.0f, 0.5f);
    ImGui::ColorEdit3("Color", (float*)&color);
    ImGui::ColorEdit3("Amb. color", (float*)&ambient_color);
}

} // namespace erwin