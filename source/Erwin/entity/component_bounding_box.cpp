#include "entity/component_bounding_box.h"
#include "imgui.h"

namespace erwin
{

ComponentOBB::ComponentOBB():
extent_m(-1.f,1.f,-1.f,1.f,-1.f,1.f),
display(false)
{
	std::tie(offset, half) = bound::to_vectors(extent_m);
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

template <> 
void inspector_GUI<ComponentOBB>(ComponentOBB* cmp)
{
    ImGui::Checkbox("Display", &cmp->display);

    for(int ii=0; ii<8; ++ii)
    	ImGui::Text("%s: (%f, %f, %f)", s_vertex_name[ii], cmp->vertices_w[ii].x, cmp->vertices_w[ii].y, cmp->vertices_w[ii].z);
}



} // namespace erwin
