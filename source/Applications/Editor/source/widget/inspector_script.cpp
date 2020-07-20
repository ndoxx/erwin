#include "entity/component/script.h"
#include "entity/reflection.h"
#include "imgui.h"

namespace erwin
{

template <> 
void inspector_GUI<ComponentScript>(ComponentScript& cmp, EntityID, entt::registry&, size_t)
{
    (void)cmp;
	ImGui::TextColored({1.f,0.75f,0.f,1.f}, "[Script]");
}

} // namespace erwin