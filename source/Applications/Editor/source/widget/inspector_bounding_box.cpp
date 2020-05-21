#include "entity/component_bounding_box.h"
#include "entity/reflection.h"
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

    for(size_t ii=0; ii<8; ++ii)
    	ImGui::Text("%s: (%f, %f, %f)", s_vertex_name[ii], double(cmp->vertices_w[ii].x), double(cmp->vertices_w[ii].y), double(cmp->vertices_w[ii].z));
}



} // namespace erwin
