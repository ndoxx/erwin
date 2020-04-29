#include "widget/widget_material_authoring.h"
#include "asset/asset_manager.h"
#include "imgui.h"
#include "imgui/font_awesome.h"
#include "render/renderer.h"

using namespace erwin;

namespace editor
{

static std::vector<std::string> s_texture_map_names = {
    "Albedo",            // TM_ALBEDO
    "Normal",            // TM_NORMAL
    "Depth",             // TM_DEPTH
    "Metal",             // TM_METAL
    "Ambient Occlusion", // TM_AO
    "Roughness",         // TM_ROUGHNESS
    "Emissivity",        // TM_EMISSIVITY
};

MaterialAuthoringWidget::MaterialAuthoringWidget() : Widget("Material authoring", true)
{
    checkerboard_tex_ = AssetManager::create_debug_texture("checkerboard"_h, 64);
}

MaterialAuthoringWidget::~MaterialAuthoringWidget() {}

static constexpr size_t k_image_size = 115;

void MaterialAuthoringWidget::on_imgui_render()
{
    // Restrict to opaque PBR materials for now
    // * For each possible texture map
    void* checkerboard_native = Renderer::get_native_texture_handle(checkerboard_tex_);
    for(TMEnum tm = 0; tm < TextureMapType::TM_COUNT; ++tm)
    {
    	bool has_map = current_composition_.has_map(TextureMapType(tm));
        ImGui::TextColored({0.5f, 0.7f, 0.f, 1.f}, "%s %s", W_ICON(PICTURE_O), s_texture_map_names[tm].c_str());

        // * Display a clickable image of the texture map
        // Display currently bound texture map
        // Default to checkerboard pattern if no texture map is loaded
	    TextureHandle tex = current_composition_.textures[size_t(tm)];
        void* image_native = has_map ? Renderer::get_native_texture_handle(tex) : checkerboard_native;

        if(image_native)
            ImGui::Image(image_native, ImVec2(k_image_size, k_image_size));
        else
        	return;

        // Context menu
        bool show_open_dialog = false;
        ImGui::PushID(int(ImGui::GetID(reinterpret_cast<void*>(intptr_t(tm)))));
        if(ImGui::BeginPopupContextItem("##TM_CONTEXT"))
        {
            if(ImGui::Selectable("Load"))
                show_open_dialog = true;

            if(has_map)
            {
	            if(ImGui::Selectable("Clear"))
	            {
	                TextureHandle tex = current_composition_.textures[size_t(tm)];
	                if(tex.is_valid())
	                {
	                	Renderer::destroy(tex);
	                	current_composition_.clear_map(TextureMapType(tm));
	                }
	            }
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();

        if(show_open_dialog)
        {
        	DLOG("editor",1) << "Loading texture map: " << s_texture_map_names[tm] << std::endl;
        }

        ImGui::Separator();
    }
}

} // namespace editor