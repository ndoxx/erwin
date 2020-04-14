#include "editor/widget_materials.h"
#include "editor/font_awesome.h"
#include "level/scene.h"
#include "asset/asset_manager.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

MaterialsWidget::MaterialsWidget():
Widget("Materials", true)
{

}

void MaterialsWidget::on_imgui_render()
{
	AssetManager::visit_materials([](MaterialHandle, const std::string& name, const std::string& description)
	{
		ImGui::Text("%s %s", ICON_FA_PICTURE_O, name.c_str());
		if(description.size())
			ImGui::TextUnformatted(description.c_str());

		ImGui::Separator();
		return false;
	});
}


} // namespace editor