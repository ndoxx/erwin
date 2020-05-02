#include "widget/widget_material_authoring.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "asset/asset_manager.h"
#include "filesystem/filesystem.h"
#include "filesystem/tom_file.h"
#include "imgui.h"
#include "imgui/color.h"
#include "imgui/dialog.h"
#include "imgui/font_awesome.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "utils/future.hpp"
#include "utils/string.h"
#include "widget/widget_material_view.h"

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

static TextureMapType detect_texture_map(const std::string& name)
{
    if(name.find("albedo") != std::string::npos)
        return TM_ALBEDO;
    else if(name.find("norm") != std::string::npos)
        return TM_NORMAL;
    else if(name.find("depth") != std::string::npos)
        return TM_DEPTH;
    else if(name.find("metal") != std::string::npos)
        return TM_METAL;
    else if(name.find("ao") != std::string::npos)
        return TM_AO;
    else if(name.find("occlusion") != std::string::npos)
        return TM_AO;
    else if(name.find("rough") != std::string::npos)
        return TM_ROUGHNESS;
    else if(name.find("emissiv") != std::string::npos)
        return TM_EMISSIVITY;
    else
        return TM_COUNT;
}

struct MaterialAuthoringWidget::MaterialComposition
{
    TMEnum texture_maps = TMF_NONE;
    erwin::TextureGroup textures;
    erwin::TextureGroup packed_textures;
    uint32_t width;
    uint32_t height;

    std::string name;

    inline bool has_map(TextureMapType map_type) { return bool(texture_maps & (1 << map_type)); }
    inline void clear_map(TextureMapType map_type)
    {
        texture_maps &= ~(1 << map_type);
        textures[size_t(map_type)] = {};
    }
    inline void set_map(TextureMapType map_type, TextureHandle tex)
    {
        textures[size_t(map_type)] = tex;
        texture_maps |= (1 << map_type);
    }
};

struct MaterialAuthoringWidget::TOMExportTask
{
    TOMExportTask(std::future<PixelData>&& a, std::future<PixelData>&& nd, std::future<PixelData>&& mare_, size_t w,
                  size_t h, const fs::path& path)
        : albedo(std::move(a)), normal_depth(std::move(nd)), mare(std::move(mare_)), width(w), height(h),
          export_path(path)
    {}

    inline bool is_ready() { return ::is_ready(albedo) && ::is_ready(normal_depth) && ::is_ready(mare); }

    std::future<PixelData> albedo;
    std::future<PixelData> normal_depth;
    std::future<PixelData> mare;

    size_t width;
    size_t height;

    fs::path export_path;
};

struct PBRPackingData
{
    int flags;
};

MaterialAuthoringWidget::MaterialAuthoringWidget(MaterialViewWidget& material_view)
    : Widget("Material authoring", true), material_view_(material_view)
{
    current_composition_ = std::make_unique<MaterialComposition>();

    checkerboard_tex_ = AssetManager::create_debug_texture("checkerboard"_h, 64);
    PBR_packing_shader_ =
        Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/PBR_packing.glsl", "PBR_packing");
    packing_ubo_ =
        Renderer::create_uniform_buffer("parameters", nullptr, sizeof(PBRPackingData), UsagePattern::Dynamic);
    Renderer::shader_attach_uniform_buffer(PBR_packing_shader_, packing_ubo_);
}

MaterialAuthoringWidget::~MaterialAuthoringWidget()
{
    Renderer::destroy(PBR_packing_shader_);
    Renderer::destroy(packing_ubo_);
}

void MaterialAuthoringWidget::pack_textures()
{
    // * Render to a compatible set of PBR packed textures
    // Create an ad-hoc framebuffer to render to 3 textures
    FramebufferLayout layout{
        // RGB: Albedo, A: Scaled emissivity
        {"albedo"_h, ImageFormat::SRGB_ALPHA, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
        // RG: Compressed normal, BA: ?
        {"normal_depth"_h, ImageFormat::RGBA16_SNORM, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
        // R: Metallic, G: AO, B: Roughness, A: ?
        {"mare"_h, ImageFormat::RGBA8, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
    };
    FramebufferHandle fb =
        Renderer::create_framebuffer(current_composition_->width, current_composition_->height, FB_NONE, layout);

    PBRPackingData data{int(current_composition_->texture_maps)};

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
    dc.add_dependency(Renderer::update_uniform_buffer(packing_ubo_, static_cast<void*>(&data), sizeof(PBRPackingData),
                                                      DataOwnership::Copy));

    Renderer::submit(key.encode(), dc);

    TextureGroup tg;
    for(uint32_t ii = 0; ii < 3; ++ii)
        tg[ii] = Renderer::get_framebuffer_texture(fb, ii);
    tg.texture_count = 3;

    Renderer::destroy(fb, true); // Destroy FB but keep attachments alive

    // * Create material and send to Material View widget
    current_composition_->packed_textures = tg;
    const Material& mat = AssetManager::create_PBR_material(current_composition_->name, tg);
    ComponentPBRMaterial cmaterial;
    cmaterial.set_material(mat);
    for(TMEnum tm = 0; tm < TextureMapType::TM_COUNT; ++tm)
        cmaterial.enable_flag(TextureMapFlag(1 << tm), current_composition_->has_map(TextureMapType(tm)));

    // TODO: Implement an interface for each texture map to adjust uniform data
    // cmaterial.material_data.uniform_roughness = ...;

    material_view_.set_material(cmaterial);
}

void MaterialAuthoringWidget::load_texture_map(TextureMapType tm_type, const fs::path& filepath)
{
    DLOG("editor", 1) << "Loading texture map: " << s_texture_map_names[size_t(tm_type)] << std::endl;
    DLOGI << WCC('p') << filepath << std::endl;

    // If texture map already assigned, delete it
    if(current_composition_->has_map(tm_type))
        clear_texture_map(tm_type);

    Texture2DDescriptor desc;
    TextureHandle tex = AssetManager::load_image(filepath, desc);
    current_composition_->set_map(tm_type, tex);
    current_composition_->width = desc.width;
    current_composition_->height = desc.height;
}

// Load all textures in a folder, try to recognize file names to assign texture maps correctly
void MaterialAuthoringWidget::load_directory(const fs::path& dirpath)
{
    clear();

    DLOG("editor", 1) << "Loading texture directory:" << std::endl;
    DLOGI << WCC('p') << dirpath << std::endl;

    bool success = false;
    // For each file in directory
    for(auto& entry : fs::directory_iterator(dirpath))
    {
        // If file is a PNG image
        if(entry.is_regular_file() && !entry.path().extension().string().compare(".png"))
        {
            // Extract name without extension, transform to lower case,
            // detect corresponding texture map type and assign
            std::string stem = entry.path().stem().string();
            su::to_lower(stem);
            TextureMapType tm_type = detect_texture_map(stem);
            if(tm_type != TM_COUNT)
            {
                load_texture_map(tm_type, entry.path());
                success = true; // Succeed if at least one texture map has been assigned
            }
        }
    }

    if(success)
    {
        // Extract directory name and make it the material name
        current_composition_->name = dirpath.stem().string();
        DLOG("editor", 1) << "Selected \"" << WCC('n') << current_composition_->name << "\" as a material name."
                          << std::endl;
    }
}

void MaterialAuthoringWidget::clear_texture_map(TextureMapType tm_type)
{
    TextureHandle tex = current_composition_->textures[size_t(tm_type)];
    if(tex.is_valid())
    {
        Renderer::destroy(tex);
        current_composition_->clear_map(tm_type);
    }
}

void MaterialAuthoringWidget::clear()
{
    for(TMEnum tm = 0; tm < TextureMapType::TM_COUNT; ++tm)
        clear_texture_map(TextureMapType(tm));

    current_composition_->name = "";
    current_composition_->packed_textures = {};
    material_view_.reset_material();
}

void MaterialAuthoringWidget::export_TOM(const fs::path& tom_path)
{
    // * Create an export task that will execute when we receive data
    auto fut_a = Renderer::get_pixel_data(current_composition_->packed_textures[0]);
    auto fut_nd = Renderer::get_pixel_data(current_composition_->packed_textures[1]);
    auto fut_mare = Renderer::get_pixel_data(current_composition_->packed_textures[2]);
    tom_export_tasks_.push_back(std::make_unique<TOMExportTask>(std::move(fut_a), std::move(fut_nd),
                                                                std::move(fut_mare), current_composition_->width,
                                                                current_composition_->height, tom_path));
}

static void handle_tom_export(const fs::path& path, size_t width, size_t height, uint8_t* albedo, uint8_t* normal_depth, uint8_t* mare)
{
    tom::TOMDescriptor tom_desc
    {
        path,
        uint16_t(width),
        uint16_t(height),
        tom::LosslessCompression::Deflate,
        TextureWrap::REPEAT
    };

    uint32_t size = uint32_t(width * height * 4);

    tom::TextureMapDescriptor albedo_desc
    {
        TextureFilter(MAG_LINEAR | MIN_LINEAR_MIPMAP_NEAREST),
        4,
        true, // sRGB
        TextureCompression::None, // TODO: Implement DXT5 compression pass in tom_file.cpp, Fudge did it separately. 
        size,
        albedo,
        "albedo"_h
    };

    tom::TextureMapDescriptor nd_desc
    {
        TextureFilter(MAG_LINEAR | MIN_LINEAR_MIPMAP_NEAREST),
        4,
        false,
        TextureCompression::None,
        size,
        normal_depth,
        "normal_depth"_h
    };

    tom::TextureMapDescriptor mare_desc
    {
        TextureFilter(MAG_LINEAR | MIN_LINEAR_MIPMAP_NEAREST),
        4,
        false,
        TextureCompression::None,
        size,
        mare,
        "mare"_h
    };

    tom_desc.texture_maps.push_back(albedo_desc);
    tom_desc.texture_maps.push_back(nd_desc);
    tom_desc.texture_maps.push_back(mare_desc);
    tom::write_tom(tom_desc);
}

void MaterialAuthoringWidget::on_update(const erwin::GameClock& clock)
{
    // * Run export tasks when they are ready
    for(auto it = tom_export_tasks_.begin(); it != tom_export_tasks_.end();)
    {
        auto&& task = *it;
        if(task->is_ready())
        {
            auto&& [a_data, a_size] = task->albedo.get();
            auto&& [nd_data, nd_size] = task->normal_depth.get();
            auto&& [mare_data, mare_size] = task->mare.get();

            DLOG("editor", 1) << "Exporting to:" << std::endl;
            DLOGI << WCC('p') << task->export_path << std::endl;

            handle_tom_export(task->export_path, task->width, task->height, a_data, nd_data, mare_data);

            delete[] a_data;
            delete[] nd_data;
            delete[] mare_data;

            tom_export_tasks_.erase(it);
        }
        else
            ++it;
    }
}

static constexpr size_t k_image_size = 100;
void MaterialAuthoringWidget::on_imgui_render()
{
    // Restrict to opaque PBR materials for now

    // * Global interface
    static std::string asset_dir = filesystem::get_asset_dir().string() + "/textures/map/";

    static char name_buf[128] = "";
    if(ImGui::InputTextWithHint("Name", current_composition_->name.c_str(), name_buf, IM_ARRAYSIZE(name_buf)))
        current_composition_->name = name_buf;

    ImVec2 btn_span_size(ImGui::GetContentRegionAvailWidth(), 0.f);
    // Load whole directory
    ImGui::PushStyleColor(ImGuiCol_Button, imgui_rgb(102, 153, 255));
    if(ImGui::Button("Load directory", btn_span_size))
    {
        ImGui::SetNextWindowSize({700, 400});
        igfd::ImGuiFileDialog::Instance()->OpenModal("ChooseDirectoryDlgKey", "Choose Directory", 0, asset_dir, "");
    }
    ImGui::PopStyleColor(1);

    if(igfd::ImGuiFileDialog::Instance()->FileDialog("ChooseDirectoryDlgKey"))
    {
        // action if OK
        if(igfd::ImGuiFileDialog::Instance()->IsOk == true)
            load_directory(igfd::ImGuiFileDialog::Instance()->GetFilepathName());
        // close
        igfd::ImGuiFileDialog::Instance()->CloseDialog("ChooseDirectoryDlgKey");
    }

    // Pack textures and apply to material view widget
    ImVec2 btn_half_span_size(ImGui::GetContentRegionAvailWidth() * 0.5f, 0.f);
    ImGui::PushStyleColor(ImGuiCol_Button, imgui_rgb(153, 204, 0));
    if(ImGui::Button("Generate", btn_half_span_size))
        pack_textures();
    ImGui::PopStyleColor(1);

    // Pack textures and apply to material view widget
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, imgui_rgb(255, 153, 0));
    if(ImGui::Button("Clear", btn_half_span_size))
        ImGui::OpenModal("Confirm");
    if(ImGui::CheckYesNo("Confirm", "Are you sure?") == ImGui::DialogState::YES)
        clear();
    ImGui::PopStyleColor(1);

    // Export material as TOM file
    ImGui::PushStyleColor(ImGuiCol_Button, imgui_rgb(102, 153, 255));
    if(ImGui::Button("Export", btn_span_size))
    {
        // Current material must have been applied
        if(current_composition_->packed_textures.texture_count > 0)
        {
            std::string default_filename = current_composition_->name + ".tom";
            ImGui::SetNextWindowSize({700, 400});
            igfd::ImGuiFileDialog::Instance()->OpenModal("ExportFileDlgKey", "Export", ".tom", asset_dir, default_filename);
        }
    }
    ImGui::PopStyleColor(1);

    if(igfd::ImGuiFileDialog::Instance()->FileDialog("ExportFileDlgKey"))
    {
        // action if OK
        if(igfd::ImGuiFileDialog::Instance()->IsOk == true)
            export_TOM(igfd::ImGuiFileDialog::Instance()->GetFilepathName());
        // close
        igfd::ImGuiFileDialog::Instance()->CloseDialog("ExportFileDlgKey");
    }

    ImGui::Separator();

    // * For each possible texture map
    bool show_file_open_dialog = false;
    static TMEnum selected_tm = 0;
    void* checkerboard_native = Renderer::get_native_texture_handle(checkerboard_tex_);
    for(TMEnum tm = 0; tm < TextureMapType::TM_COUNT; ++tm)
    {
        bool has_map = current_composition_->has_map(TextureMapType(tm));
        ImGui::Text("%s %s", W_ICON(PICTURE_O), s_texture_map_names[tm].c_str());

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
                show_file_open_dialog = true;
                selected_tm = tm;
            }

            if(has_map)
            {
                if(ImGui::Selectable("Clear"))
                    clear_texture_map(TextureMapType(tm));
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
        ImGui::Separator();
    }

    if(show_file_open_dialog)
    {
        ImGui::SetNextWindowSize({700, 400});
        igfd::ImGuiFileDialog::Instance()->OpenModal("ChooseFileDlgKey", "Choose File", ".png", asset_dir, "");
    }
    if(igfd::ImGuiFileDialog::Instance()->FileDialog("ChooseFileDlgKey"))
    {
        // action if OK
        if(igfd::ImGuiFileDialog::Instance()->IsOk == true)
        {
            fs::path filepath = igfd::ImGuiFileDialog::Instance()->GetFilepathName();
            load_texture_map(TextureMapType(selected_tm), filepath);
            // Extract parent directory name and make it the material name
            current_composition_->name = filepath.parent_path().stem().string();
        }
        // close
        igfd::ImGuiFileDialog::Instance()->CloseDialog("ChooseFileDlgKey");
    }
}

} // namespace editor