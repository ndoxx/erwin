#include "entity/component/bounding_box.h"
#include "entity/reflection.h"
#include "imgui.h"

namespace erwin
{

static const char* s_vertex_name[8] =
{
	"RBF", "RTF", "LTF", "LBF",
	"RBB", "RTB", "LTB", "LBB"
};

template <> 
void inspector_GUI<ComponentOBB>(ComponentOBB& cmp, EntityID, entt::registry&, size_t)
{
    ImGui::Checkbox("Display", &cmp.display);

    for(size_t ii=0; ii<8; ++ii)
    	ImGui::Text("%s: (%f, %f, %f)", s_vertex_name[ii], double(cmp.vertices_w[ii].x), double(cmp.vertices_w[ii].y), double(cmp.vertices_w[ii].z));
}



} // namespace erwin
