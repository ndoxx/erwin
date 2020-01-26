#include "entity/component_bounding_box.h"
#include "imgui.h"

namespace erwin
{

COMPONENT_DEFINITION(ComponentOBB);
bool ComponentOBB::init(void* description)
{

	return true;
}

ComponentOBB::ComponentOBB(const Extent& extent):
extent_m(extent)
{
	std::tie(offset, half) = bound::to_vectors(extent_m);
}

void ComponentOBB::inspector_GUI()
{
    ImGui::TextColored({0.f,0.75f,1.f,1.f}, "ComponentOBB");

    // TMP
    for(int ii=0; ii<8; ++ii)
    {
    	ImGui::Text("(%f, %f, %f)", vertices_w[ii].x, vertices_w[ii].y, vertices_w[ii].z);
    }
}



} // namespace erwin
