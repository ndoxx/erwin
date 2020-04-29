#include "widget/widget_material_authoring.h"
#include "widget/widget_material_view.h"
#include "asset/asset_manager.h"
#include "render/renderer.h"
#include "render/common_geometry.h"
#include "filesystem/filesystem.h"
#include "imgui.h"
#include "imgui/font_awesome.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

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

struct MaterialAuthoringWidget::MaterialComposition
{
	TMEnum texture_maps = TMF_NONE;
	erwin::TextureGroup textures;
	uint32_t width;
	uint32_t height;

    inline bool has_map(TextureMapType map_type)   { return bool(texture_maps & (1 << map_type)); }
    inline void clear_map(TextureMapType map_type) { texture_maps &= ~(1 << map_type); textures[size_t(map_type)] = {}; }
    inline void set_map(TextureMapType map_type, TextureHandle tex)
    {
    	textures[size_t(map_type)] = tex;
    	texture_maps |= (1 << map_type);
    }
};

struct PBRPackingData
{
	int flags;
};

MaterialAuthoringWidget::MaterialAuthoringWidget(MaterialViewWidget& material_view):
Widget("Material authoring", true),
material_view_(material_view)
{
    current_composition_ = std::make_unique<MaterialComposition>();

    checkerboard_tex_ = AssetManager::create_debug_texture("checkerboard"_h, 64);
    PBR_packing_shader_ = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/PBR_packing.glsl", "PBR_packing");
	packing_ubo_  = Renderer::create_uniform_buffer("parameters", nullptr, sizeof(PBRPackingData), UsagePattern::Dynamic);
	Renderer::shader_attach_uniform_buffer(PBR_packing_shader_, packing_ubo_);
}

MaterialAuthoringWidget::~MaterialAuthoringWidget()
{
	Renderer::destroy(PBR_packing_shader_);
	Renderer::destroy(packing_ubo_);
}

TextureGroup MaterialAuthoringWidget::pack_textures()
{
	// Create an ad-hoc framebuffer to render to 3 textures
    FramebufferLayout layout
    {
    	// RGB: Albedo, A: Scaled emissivity
        {"albedo"_h, ImageFormat::RGBA8, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
        // RG: Compressed normal, BA: ?
        {"normal_depth"_h, ImageFormat::RGBA8, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
        // R: Metallic, G: AO, B: Roughness, A: ?
        {"mare"_h, ImageFormat::RGBA8, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
    };
	FramebufferHandle fb = Renderer::create_framebuffer(current_composition_->width, current_composition_->height, FB_NONE, layout);

	PBRPackingData data { int(current_composition_->texture_maps) };

	// Render a single quad
	RenderState state;
	state.render_target = fb;
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = false;
	state.depth_stencil_state.depth_lock = true;

	uint64_t state_flags = state.encode();

	SortKey key;
	key.set_sequence(0, 0, PBR_packing_shader_);

	VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);
	DrawCall dc(DrawCall::Indexed, state_flags, PBR_packing_shader_, quad);
	for(TMEnum tm = 0; tm < TextureMapType::TM_COUNT; ++tm)
		dc.set_texture(current_composition_->textures[size_t(tm)], uint32_t(tm));
	dc.add_dependency(Renderer::update_uniform_buffer(packing_ubo_, static_cast<void*>(&data), sizeof(PBRPackingData), DataOwnership::Copy));

	Renderer::submit(key.encode(), dc);

	TextureGroup tg;
	for(uint32_t ii=0; ii<3; ++ii)
		tg[ii] = Renderer::get_framebuffer_texture(fb, ii);
	tg.texture_count = 3;

	Renderer::destroy(fb, true); // Destroy FB but keep cubemap attachment alive

	return tg;
}

static constexpr size_t k_image_size = 115;

void MaterialAuthoringWidget::on_imgui_render()
{
    // Restrict to opaque PBR materials for now

	// * Global interface
	if(ImGui::Button("Apply"))
	{
		TextureGroup tg = pack_textures();
		const Material& mat = AssetManager::create_PBR_material("TestMat", tg);
		ComponentPBRMaterial cmaterial;
		cmaterial.set_material(mat);
    	for(TMEnum tm = 0; tm < TextureMapType::TM_COUNT; ++tm)
    		cmaterial.enable_flag(TextureMapFlag(1<<tm), current_composition_->has_map(TextureMapType(tm)));

    	cmaterial.material_data.uniform_roughness = 0.1f;

    	material_view_.set_material(cmaterial);
	}

	ImGui::Separator();

    // * For each possible texture map
    bool show_open_dialog = false;
    static TMEnum selected_tm = 0;
    void* checkerboard_native = Renderer::get_native_texture_handle(checkerboard_tex_);
    for(TMEnum tm = 0; tm < TextureMapType::TM_COUNT; ++tm)
    {
    	bool has_map = current_composition_->has_map(TextureMapType(tm));
        ImGui::TextColored({0.5f, 0.7f, 0.f, 1.f}, "%s %s", W_ICON(PICTURE_O), s_texture_map_names[tm].c_str());

        // * Display a clickable image of the texture map
        // Display currently bound texture map
        // Default to checkerboard pattern if no texture map is loaded
	    TextureHandle tex = current_composition_->textures[size_t(tm)];
        void* image_native = has_map ? Renderer::get_native_texture_handle(tex) : checkerboard_native;

        if(image_native)
            ImGui::Image(image_native, ImVec2(k_image_size, k_image_size));
        else
        	return;

        // Context menu
        ImGui::PushID(int(ImGui::GetID(reinterpret_cast<void*>(intptr_t(tm)))));
        if(ImGui::BeginPopupContextItem("##TM_CONTEXT"))
        {
            if(ImGui::Selectable("Load"))
            {
                show_open_dialog = true;
                selected_tm = tm;
            }

            if(has_map)
            {
	            if(ImGui::Selectable("Clear"))
	            {
	                TextureHandle tex = current_composition_->textures[size_t(tm)];
	                if(tex.is_valid())
	                {
	                	Renderer::destroy(tex);
	                	current_composition_->clear_map(TextureMapType(tm));
	                }
	            }
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
        ImGui::Separator();
    }

    if(show_open_dialog)
    {
    	// const std::string& dir = (filesystem::get_asset_dir() / "textures/map/upack/").string();
    	igfd::ImGuiFileDialog::Instance()->OpenModal("ChooseFileDlgKey", "Choose File", ".png", ".");
    }
    if(igfd::ImGuiFileDialog::Instance()->FileDialog("ChooseFileDlgKey"))
    {
        // action if OK
        if(igfd::ImGuiFileDialog::Instance()->IsOk == true)
        {
            std::string filepath = igfd::ImGuiFileDialog::Instance()->GetFilepathName();
    		DLOG("editor",1) << "Loading texture map: " << s_texture_map_names[selected_tm] << std::endl;
    		DLOGI << "Path: " << WCC('p') << filepath << std::endl;
    		Texture2DDescriptor desc;
    		TextureHandle tex = AssetManager::load_image(filepath, desc);
    		current_composition_->set_map(TextureMapType(selected_tm), tex);
    		current_composition_->width = desc.width;
    		current_composition_->height = desc.height;
        }
        // close
        igfd::ImGuiFileDialog::Instance()->CloseDialog("ChooseFileDlgKey");
    }
}

} // namespace editor