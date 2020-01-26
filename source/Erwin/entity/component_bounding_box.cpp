#include "entity/component_bounding_box.h"
#include "imgui.h"

namespace erwin
{

COMPONENT_DEFINITION(ComponentOBB);
bool ComponentOBB::init(void* description)
{

	return true;
}

ComponentOBB::ComponentOBB():
display(false)
{

}

ComponentOBB::ComponentOBB(const Extent& extent):
extent_m(extent),
display(false)
{
	std::tie(offset, half) = bound::to_vectors(extent_m);
}

static const char* s_vertex_name[8] =
{
	"RBF", "RTF", "LTF", "LBF",
	"RBB", "RTB", "LTB", "LBB"
};

void ComponentOBB::inspector_GUI()
{
    ImGui::Checkbox("Display", &display);

    for(int ii=0; ii<8; ++ii)
    	ImGui::Text("%s: (%f, %f, %f)", s_vertex_name[ii], vertices_w[ii].x, vertices_w[ii].y, vertices_w[ii].z);
}



} // namespace erwin
