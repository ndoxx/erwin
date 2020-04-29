#include "widget/widget_material_authoring.h"
#include "render/renderer.h"
#include "asset/asset_manager.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{


MaterialAuthoringWidget::MaterialAuthoringWidget():
Widget("Material authoring", true)
{
	checkerboard_tex_ = AssetManager::create_debug_texture("checkerboard"_h, 64);
}

MaterialAuthoringWidget::~MaterialAuthoringWidget()
{
	
}

void MaterialAuthoringWidget::on_imgui_render()
{
	// Restrict to opaque PBR materials for now
	// * For each possible texture map

	// * Display a clickable image of the texture map
	// Default to checkerboard pattern if no texture map is loaded
    void* checkerboard_native = Renderer::get_native_texture_handle(checkerboard_tex_);
    if(checkerboard_native)
    {
		ImGui::Image(checkerboard_native, ImVec2(64, 64));
	}
	// Display currently bound texture map otherwise
}


} // namespace editor