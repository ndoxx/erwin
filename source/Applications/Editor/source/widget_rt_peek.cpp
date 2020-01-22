#include "widget_rt_peek.h"
#include "erwin.h"
#include "scene.h"
#include "core/intern_string.h"
#include "imgui.h"

#include <vector>

using namespace erwin;

namespace editor
{

struct DebugTextureProperties
{
    TextureHandle texture;
    bool is_depth;
    std::string name;
};

struct DebugPane
{
    std::string name;
    std::vector<DebugTextureProperties> properties;
};

enum PeekFlags: uint32_t
{
    NONE = 0,
    TONE_MAP = 1,
    SPLIT_ALPHA = 2,
    INVERT = 4,
    DEPTH = 8,
};

struct PeekData
{
    uint32_t flags = 0;
    float split_pos = 0.5f;
    glm::vec2 texel_size;
    glm::vec4 channel_filter;
    glm::vec4 projection_parameters;
};

static struct
{
    std::vector<DebugPane> panes_;
    int current_pane_;
    int current_tex_;
    bool tone_map_;
    bool show_r_;
    bool show_g_;
    bool show_b_;
    bool invert_color_;
    bool split_alpha_;
    bool save_image_;

    ShaderHandle peek_shader_;
    UniformBufferHandle pass_ubo_;
    uint64_t pass_state_;

    PeekData peek_data_;
} s_storage;

RTPeekWidget::RTPeekWidget(Scene& scene):
Widget("Framebuffers", true),
scene_(scene)
{
    // Create resources
    FramebufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    FramebufferPool::create_framebuffer("fb_texture_view"_h, make_scope<FbRatioConstraint>(), layout, false);

    // s_storage.peek_shader_ = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/texture_peek.glsl", "texture_peek");
    s_storage.peek_shader_ = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/texture_peek.spv", "texture_peek");
    s_storage.pass_ubo_ = Renderer::create_uniform_buffer("peek_layout", nullptr, sizeof(PeekData), UsagePattern::Dynamic);
    Renderer::shader_attach_uniform_buffer(s_storage.peek_shader_, s_storage.pass_ubo_);

    // Initialize GUI
    s_storage.current_pane_ = 0;
    s_storage.current_tex_ = 0;
    s_storage.save_image_ = false;
    s_storage.show_r_ = true;
    s_storage.show_g_ = true;
    s_storage.show_b_ = true;

    // State
    RenderState state;
    state.render_target = FramebufferPool::get_framebuffer("fb_texture_view"_h);
    state.rasterizer_state.cull_mode = CullMode::Back;
    state.rasterizer_state.clear_flags = CLEAR_COLOR_FLAG;
    state.blend_state = BlendState::Opaque;
    state.depth_stencil_state.depth_test_enabled = false;
    s_storage.pass_state_ = state.encode();
}

RTPeekWidget::~RTPeekWidget()
{

}

uint32_t RTPeekWidget::new_pane(const std::string& name)
{
    uint32_t pane_index = s_storage.panes_.size();
    s_storage.panes_.push_back(DebugPane{name});
    return pane_index;
}

void RTPeekWidget::register_texture(uint32_t pane_index, TextureHandle texture, const std::string& name, bool is_depth)
{
    W_ASSERT(pane_index < s_storage.panes_.size(), "Pane index out of bounds.");
    s_storage.panes_[pane_index].properties.push_back({texture, is_depth, name});
}

void RTPeekWidget::register_framebuffer(const std::string& framebuffer_name)
{
    hash_t hframebuffer = H_(framebuffer_name.c_str());
    FramebufferHandle fb = FramebufferPool::get_framebuffer(hframebuffer);
    bool has_depth = FramebufferPool::has_depth(hframebuffer);
    uint32_t ntex = Renderer::get_framebuffer_texture_count(fb);
    uint32_t pane_index = new_pane(framebuffer_name);

    for(uint32_t ii=0; ii<ntex; ++ii)
    {
        TextureHandle texture_handle = Renderer::get_framebuffer_texture(fb, ii);
        hash_t hname = Renderer::get_framebuffer_texture_name(fb, ii);
        std::string tex_name = istr::resolve(hname);
        register_texture(pane_index, texture_handle, tex_name, (has_depth && ii==ntex-1));
    }
}

void RTPeekWidget::on_layer_render()
{
    if(!open_ || s_storage.panes_.size() == 0)
        return;

    s_storage.peek_data_.projection_parameters = scene_.camera_controller.get_camera().get_projection_parameters();

    DebugTextureProperties& props = s_storage.panes_[s_storage.current_pane_].properties[s_storage.current_tex_];
    TextureHandle current_texture = props.texture;
    
    // Update UBO data
    s_storage.peek_data_.texel_size = FramebufferPool::get_texel_size("fb_texture_view"_h);
    s_storage.peek_data_.flags = (s_storage.tone_map_ ? PeekFlags::TONE_MAP : PeekFlags::NONE)
                               | (s_storage.invert_color_ ? PeekFlags::INVERT : PeekFlags::NONE)
                               | (s_storage.split_alpha_ ? PeekFlags::SPLIT_ALPHA : PeekFlags::NONE)
                               | (props.is_depth ? PeekFlags::DEPTH : PeekFlags::NONE);
    s_storage.peek_data_.channel_filter = { s_storage.show_r_, s_storage.show_g_, s_storage.show_b_, 1.f };

    // Submit draw call
    // WTF: If layer_id is set, command is dispatched at the end of frame like I would want it to,
    // but it fails miserably and the whole GUI disappears. I observed that setting the key this way
    // dispatches the command at the beginning of the frame, which works, despite showing the last
    // frame textures.
    SortKey key;
    key.set_sequence(0, 0, s_storage.peek_shader_);
    DrawCall dc(DrawCall::Indexed, s_storage.pass_state_, s_storage.peek_shader_, CommonGeometry::get_vertex_array("quad"_h));
    dc.add_dependency(Renderer::update_uniform_buffer(s_storage.pass_ubo_, &s_storage.peek_data_, sizeof(PeekData), DataOwnership::Copy));
    dc.set_UBO(s_storage.pass_ubo_);
    dc.set_texture(current_texture);
    Renderer::submit(key.encode(), dc);
}

void RTPeekWidget::on_imgui_render()
{
    if(s_storage.panes_.size() == 0)
        return;

    // * Get render properties from GUI
    // Select pane (render target)
    ImGui::BeginChild("##peekctl", ImVec2(0, 4*ImGui::GetTextLineHeightWithSpacing()));
    ImGui::Columns(2, nullptr, false);

    static const char* cur_target_item = s_storage.panes_[s_storage.current_pane_].name.data();
    static const char* cur_tex_item = s_storage.panes_[s_storage.current_pane_].properties[0].name.data();

    if(ImGui::BeginCombo("Target", cur_target_item))
    {
        for(int ii=0; ii<s_storage.panes_.size(); ++ii)
        {
            const DebugPane& pane = s_storage.panes_[ii];
            bool is_selected = (cur_target_item == pane.name.data());
            if(ImGui::Selectable(pane.name.data(), is_selected))
            {
                cur_target_item = pane.name.data();
                s_storage.current_pane_ = ii;
                s_storage.current_tex_ = 0;
                cur_tex_item = pane.properties[0].name.data();
            }
            if(is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    int ntex = s_storage.panes_[s_storage.current_pane_].properties.size();
    const DebugPane& pane = s_storage.panes_[s_storage.current_pane_];

    // Select attachment
    if(ImGui::BeginCombo("Texture", cur_tex_item))
    {
        for(int ii=0; ii<ntex; ++ii)
        {
            const DebugTextureProperties& props = pane.properties[ii];
            bool is_selected = (cur_tex_item == props.name.data());
            if(ImGui::Selectable(props.name.data(), is_selected))
            {
                cur_tex_item = props.name.data();
                s_storage.current_tex_ = ii;
            }
            if(is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    DebugTextureProperties& props = s_storage.panes_[s_storage.current_pane_].properties[s_storage.current_tex_];
    if(ImGui::Button("Save to file"))
        s_storage.save_image_ = true;
    
    if(!props.is_depth)
    {
        ImGui::NextColumn();
        ImGui::Checkbox("Tone map", &s_storage.tone_map_);
        ImGui::SameLine(); ImGui::Selectable("R##fbp_chan", &s_storage.show_r_, 0, ImVec2(15,15));
        ImGui::SameLine(); ImGui::Selectable("G##fbp_chan", &s_storage.show_g_, 0, ImVec2(15,15));
        ImGui::SameLine(); ImGui::Selectable("B##fbp_chan", &s_storage.show_b_, 0, ImVec2(15,15));
        ImGui::SameLine(); ImGui::Selectable("I##fbp_inve", &s_storage.invert_color_, 0, ImVec2(15,15));

        ImGui::Checkbox("Alpha split", &s_storage.split_alpha_);
        if(s_storage.split_alpha_)
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::SameLine();
            ImGui::DragFloat("##split_pos", &s_storage.peek_data_.split_pos, 0.01f, 0.f, 1.f);
        }
    }

    ImGui::EndChild();

    // * Show image in window
    float winx = std::max(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - 8.f, 0.f);
    float winy = std::max(ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - 8.f, 0.f);

    // Retrieve the native framebuffer texture handle
    FramebufferHandle fb = FramebufferPool::get_framebuffer("fb_texture_view"_h);
    TextureHandle texture = Renderer::get_framebuffer_texture(fb, 0);
    void* framebuffer_texture_native = Renderer::get_native_texture_handle(texture);
    ImGui::GetWindowDrawList()->AddImage(framebuffer_texture_native,
                                         ImGui::GetCursorScreenPos(),
                                         ImVec2(winx, winy),
                                         ImVec2(0, 1), ImVec2(1, 0));

    // * Save image if needed
    if(s_storage.save_image_)
    {
        std::string filename = props.name + ".png";
        DLOG("editor",1) << "Saving framebuffer texture as image: " << std::endl;
        DLOGI << WCC('p') << filename << std::endl;
        Renderer::framebuffer_screenshot(fb, filename);
        s_storage.save_image_ = false;
    }
}


} // namespace editor