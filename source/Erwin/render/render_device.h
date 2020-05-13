#pragma once

#include <cstdint>
#include <memory>
#include <future>

#include "core/core.h"
#include "render/render_state.h"
#include "render/buffer_layout.h"
#include "render/handles.h"
#include "render/texture_common.h"
#include "memory/memory.hpp"

namespace erwin
{

enum class GfxAPI
{
    None = 0,
    OpenGL = 1
};

struct FramebufferTextureVector
{
    std::vector<TextureHandle> handles;
    std::vector<hash_t> debug_names;
    CubemapHandle cubemap;
};

struct ShaderCompatibility
{
    bool ready = false;
    BufferLayout layout;

    inline void set_layout(const BufferLayout& _layout)
    {
        layout = _layout;
        ready = true;
    }

    inline void clear()
    {
        layout.clear();
        ready = false;
    }
};

// Following API is subject to future HEAVY changes
class RenderDevice
{
public:
    virtual ~RenderDevice() = default;
    virtual void release() = 0;

    // * Framebuffer
    // Add framebuffer texture description
    virtual void add_framebuffer_texture_vector(FramebufferHandle handle, const FramebufferTextureVector& ftv) = 0;
    // Bind the default framebuffer
    virtual void bind_default_framebuffer() = 0;
    // Read framebuffer content to an array
    virtual void read_framebuffer_rgba(uint32_t width, uint32_t height, unsigned char* pixels) = 0;

    // Promise texture data
    virtual std::pair<uint64_t, std::future<PixelData>> future_texture_data() = 0;

    virtual FramebufferHandle default_render_target() = 0;
    virtual TextureHandle get_framebuffer_texture(FramebufferHandle handle, uint32_t index) = 0;
    virtual CubemapHandle get_framebuffer_cubemap(FramebufferHandle handle) = 0;
    virtual hash_t get_framebuffer_texture_name(FramebufferHandle handle, uint32_t index) = 0;
    virtual uint32_t get_framebuffer_texture_count(FramebufferHandle handle) = 0;
    virtual void* get_native_texture_handle(TextureHandle handle) = 0;
    virtual VertexBufferLayoutHandle create_vertex_buffer_layout(const std::vector<BufferLayoutElement>& elements) = 0;
    virtual const BufferLayout& get_vertex_buffer_layout(VertexBufferLayoutHandle handle) = 0;

    // * Command dispatch
    virtual void dispatch_command(uint16_t type, memory::LinearBuffer<>& buf) = 0;
    virtual void dispatch_draw(uint16_t type, memory::LinearBuffer<>& buf) = 0;

    // ------------- REMOVE -------------
    // Draw content of specified vertex array using indices
    virtual void draw_indexed(void* vertexArray,
                              uint32_t count = 0,
                              std::size_t offset = 0) = 0;
    // Draw content of vertex array using only vertex buffer data
    virtual void draw_array(void* vertexArray,
                            DrawPrimitive prim = DrawPrimitive::Triangles,
                            uint32_t count = 0,
                            std::size_t offset = 0) = 0;
    // Draw instance_count instances of content of vertex array using index buffer
    virtual void draw_indexed_instanced(void* vertexArray,
                                        uint32_t instance_count,
                                        uint32_t elements_count = 0,
                                        std::size_t offset = 0) = 0;
    // ------------- REMOVE -------------

    // Set the color used to clear any framebuffer
    virtual void set_clear_color(float r, float g, float b, float a) = 0;
    // Clear currently bound framebuffer
    virtual void clear(int flags) = 0;
    // Prevent from drawing in current framebuffer's color attachment(s)
    virtual void lock_color_buffer() = 0;

    // * Global state
    virtual void set_seamless_cubemaps_enabled(bool value) = 0;

    // * Depth-stencil state
    // Lock/Unlock writing to the current framebuffer's depth buffer
    virtual void set_depth_lock(bool value) = 0;
    // Lock/Unlock writing to the current framebuffer's stencil
    virtual void set_stencil_lock(bool value) = 0;
    // Set function used as a depth test
    virtual void set_depth_func(DepthFunc value) = 0;
    // Enable/Disable depth test
    virtual void set_depth_test_enabled(bool value) = 0;
    // Set function used as a stencil test, with a reference value and a mask
    virtual void set_stencil_func(StencilFunc value, uint16_t a=0, uint16_t b=0) = 0;
    // Specify front/back stencil test action
    virtual void set_stencil_operator(StencilOperator value) = 0;
    // Enable/Disable stencil test
    virtual void set_stencil_test_enabled(bool value) = 0;

    // * Blending
    // Enable blending with blending equation dst.rgb = src.a*src.rgb + (1-src.a)*dst.rgb
    virtual void set_std_blending() = 0;
    // Enable blending with blending equation dst.rgb = src.rgb + dst.rgb
    virtual void set_light_blending() = 0;
    // Disable blending
    virtual void disable_blending() = 0;

    // * Byte-alignment
    // Specify alignment constraint on pixel rows when data is fed to client (value must be in {1,2,4,8})
    virtual void set_pack_alignment(uint32_t value) = 0;
    // Specify alignment constraint on pixel rows when data is read from client (value must be in {1,2,4,8})
    virtual void set_unpack_alignment(uint32_t value) = 0;

    // * Raster state
    // Set the position and size of area to draw to
    virtual void viewport(float xx, float yy, float width, float height) = 0;
    // Set which faces to cull out (front/back/none)
    virtual void set_cull_mode(CullMode value) = 0;
    // Set the line width for next line primitive draw calls
    virtual void set_line_width(float value) = 0;

    // * Sync
    // Wait till all graphics commands have been processed by device
    virtual void finish() = 0;
    // Force issued commands to yield in a finite time
    virtual void flush() = 0;

    // * Debug
    // Get current error from graphics device
    virtual uint32_t get_error() = 0;
    virtual const std::string& show_error() = 0;
    // Fail on graphics device error
    virtual void assert_no_error() = 0;
};

class Gfx
{
public:
    inline static GfxAPI get_api() { return api_; }
    static void set_api(GfxAPI api);

    static std::unique_ptr<RenderDevice> device;

private:
    static GfxAPI api_;
};


} // namespace erwin
