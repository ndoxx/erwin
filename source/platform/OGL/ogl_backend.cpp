#include <iostream>
#include <map>

#include "core/core.h"
#include "glad/glad.h"
#include "platform/OGL/ogl_backend.h"
#include "platform/OGL/ogl_buffer.h"
#include "platform/OGL/ogl_framebuffer.h"
#include "platform/OGL/ogl_shader.h"
#include "platform/OGL/ogl_texture.h"
#include "platform/OGL/ogl_debug.h"
#include "render/commands.h"
#include "render/renderer_config.h"
#include "utils/promise_storage.hpp"
#include <kibble/math/color.h>

namespace erwin
{

static constexpr uint16_t k_invalid_index = 0xffff;

static const std::map<DrawPrimitive, GLenum> OGLPrimitive = {
    {DrawPrimitive::Lines, GL_LINES}, {DrawPrimitive::Triangles, GL_TRIANGLES}, {DrawPrimitive::Quads, GL_QUADS}};

inline bool is_power_of_2(int value) { return bool((value) && ((value & (value - 1)) == 0)); }

inline bool is_power_of_2(uint32_t value) { return is_power_of_2(int(value)); }

struct VertexArrayDependencies
{
    std::vector<VertexBufferHandle> vbos;
    IndexBufferHandle ibo = {};

    inline void release()
    {
        for(VertexBufferHandle vbo: vbos)
            vbo.release();
        if(!ibo.is_null())
            ibo.release();
        vbos.clear();
    }
};

static struct RenderDeviceStorage
{
    void init()
    {
        state_cache_ = RenderState().encode();
        clear_resources();

        default_framebuffer_ = FramebufferHandle::acquire();
        current_framebuffer_index_ = default_framebuffer_.index();
        host_window_size_ = {0, 0};

        invalidate_texture_cache();
        invalidate_cubemap_cache();
        invalidate_VAO_cache();
        invalidate_shader_cache();
    }

    void release()
    {
        default_framebuffer_.release();
        clear_resources();
    }

    inline void clear_resources()
    {
        for(auto&& obj: index_buffers) obj.release();
        for(auto&& obj: vertex_buffers) obj.release();
        for(auto&& obj: vertex_arrays) obj.release();
        for(auto&& obj: uniform_buffers) obj.release();
        for(auto&& obj: shader_storage_buffers) obj.release();
        for(auto&& obj: textures) obj.release();
        for(auto&& obj: cubemaps) obj.release();
        std::fill(std::begin(vertex_buffer_layouts), std::end(vertex_buffer_layouts), nullptr);
        std::fill(std::begin(shaders), std::end(shaders), nullptr);
        std::fill(std::begin(framebuffers), std::end(framebuffers), nullptr);
    }

    inline void invalidate_texture_cache()
    {
        std::fill(last_texture_index, last_texture_index + k_max_texture_slots, k_invalid_index);
    }

    inline void invalidate_cubemap_cache()
    {
        std::fill(last_cubemap_index, last_cubemap_index + k_max_cubemap_slots, k_invalid_index);
    }

    inline void invalidate_VAO_cache()
    {
        last_VAO_index = k_invalid_index;
    }

    inline void invalidate_shader_cache()
    {
        last_shader_index = k_invalid_index;
    }

    FramebufferHandle default_framebuffer_ = {};
    uint16_t current_framebuffer_index_ = {};
    glm::vec2 host_window_size_;
    std::map<uint16_t, FramebufferTextureVector> framebuffer_textures_;
    std::array<VertexArrayDependencies, k_max_handles<VertexArrayHandle>> vertex_array_dependencies;

    std::array<OGLIndexBuffer, k_max_handles<IndexBufferHandle>> index_buffers;
    std::array<OGLVertexBuffer, k_max_handles<VertexBufferHandle>> vertex_buffers;
    std::array<OGLVertexArray, k_max_handles<VertexArrayHandle>> vertex_arrays;
    std::array<OGLUniformBuffer, k_max_handles<UniformBufferHandle>> uniform_buffers;
    std::array<OGLShaderStorageBuffer, k_max_handles<ShaderStorageBufferHandle>> shader_storage_buffers;
    std::array<OGLTexture2D, k_max_handles<OGLTexture2D>> textures;
    std::array<OGLCubemap, k_max_handles<OGLCubemap>> cubemaps;

    WRef<BufferLayout> vertex_buffer_layouts[k_max_handles<VertexBufferLayoutHandle>];
    WRef<OGLShader> shaders[k_max_handles<ShaderHandle>];
    WRef<OGLFramebuffer> framebuffers[k_max_handles<FramebufferHandle>];

    // ShaderCompatibility shader_compat[k_max_handles<ShaderHandle>];
    PromiseStorage<PixelData> texture_data_promises_;
    uint64_t state_cache_;
    uint16_t last_shader_index;
    uint16_t last_VAO_index;
    uint16_t last_texture_index[k_max_texture_slots];
    uint16_t last_cubemap_index[k_max_cubemap_slots];
} s_storage;

OGLBackend::OGLBackend()
{
    // Init storage
    s_storage.init();
}

void OGLBackend::release() { s_storage.release(); }

// ------------------- PRIVATE API -------------------
const OGLTexture2D& OGLBackend::create_texture_inplace(TextureHandle handle, const Texture2DDescriptor& desc)
{
    auto& ret = s_storage.textures[handle.index()];
    ret.release();
    ret.init(desc);
    return ret;
}

const OGLCubemap& OGLBackend::create_cubemap_inplace(CubemapHandle handle, const CubemapDescriptor& desc)
{
    auto& ret = s_storage.cubemaps[handle.index()];
    ret.release();
    ret.init(desc);
    return ret;
}

const OGLTexture2D& OGLBackend::get_texture(TextureHandle handle)
{
    K_ASSERT(handle.is_valid(), "Invalid TextureHandle.");
    return s_storage.textures[handle.index()];
}

const OGLCubemap& OGLBackend::get_cubemap(CubemapHandle handle)
{
    K_ASSERT(handle.is_valid(), "Invalid CubemapHandle.");
    return s_storage.cubemaps[handle.index()];
}
// ------------------- PRIVATE API -------------------

void OGLBackend::viewport(float xx, float yy, float width, float height) { glViewport(xx, yy, width, height); }

void OGLBackend::add_framebuffer_texture_vector(FramebufferHandle handle, const FramebufferTextureVector& ftv)
{
    s_storage.framebuffer_textures_.insert(std::make_pair(handle.index(), ftv));
}

void OGLBackend::bind_default_framebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s_storage.current_framebuffer_index_ = s_storage.default_framebuffer_.index();
}

void OGLBackend::read_framebuffer_rgba(uint32_t width, uint32_t height, unsigned char* pixels)
{
    set_pack_alignment(1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
    set_pack_alignment(4);
}

std::pair<uint64_t, std::future<PixelData>> OGLBackend::future_texture_data()
{
    return s_storage.texture_data_promises_.future_operation();
}

FramebufferHandle OGLBackend::default_render_target() { return s_storage.default_framebuffer_; }

TextureHandle OGLBackend::get_framebuffer_texture(FramebufferHandle handle, uint32_t index)
{
    K_ASSERT(handle.is_valid(), "Invalid FramebufferHandle.");
    K_ASSERT(index < s_storage.framebuffer_textures_[handle.index()].handles.size(),
             "Invalid framebuffer texture index.");
    return s_storage.framebuffer_textures_[handle.index()].handles[index];
}

CubemapHandle OGLBackend::get_framebuffer_cubemap(FramebufferHandle handle)
{
    K_ASSERT(handle.is_valid(), "Invalid FramebufferHandle.");
    return s_storage.framebuffer_textures_[handle.index()].cubemap;
}

hash_t OGLBackend::get_framebuffer_texture_name(FramebufferHandle handle, uint32_t index)
{
    K_ASSERT(handle.is_valid(), "Invalid FramebufferHandle.");
    return s_storage.framebuffer_textures_[handle.index()].debug_names[index];
}

uint32_t OGLBackend::get_framebuffer_texture_count(FramebufferHandle handle)
{
    K_ASSERT(handle.is_valid(), "Invalid FramebufferHandle.");
    return uint32_t(s_storage.framebuffer_textures_[handle.index()].handles.size());
}

void* OGLBackend::get_native_texture_handle(TextureHandle handle)
{
    K_ASSERT(handle.is_valid(), "Invalid TextureHandle.");
    if(!s_storage.textures[handle.index()].is_initialized())
        return nullptr;
    return s_storage.textures[handle.index()].get_native_handle();
}

VertexBufferLayoutHandle OGLBackend::create_vertex_buffer_layout(const std::vector<BufferLayoutElement>& elements)
{
    VertexBufferLayoutHandle handle = VertexBufferLayoutHandle::acquire();
    K_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    s_storage.vertex_buffer_layouts[handle.index()] = make_ref<BufferLayout>(&elements[0], elements.size());

    return handle;
}

const BufferLayout& OGLBackend::get_vertex_buffer_layout(VertexBufferLayoutHandle handle)
{
    K_ASSERT(handle.is_valid(), "Invalid VertexBufferLayoutHandle!");

    return *s_storage.vertex_buffer_layouts[handle.index()];
}

void OGLBackend::set_clear_color(float r, float g, float b, float a) { glClearColor(r, g, b, a); }

void OGLBackend::clear(int flags)
{
    GLenum clear_bits = 0;
    clear_bits |= (flags & CLEAR_COLOR_FLAG) ? GL_COLOR_BUFFER_BIT : 0;
    clear_bits |= (flags & CLEAR_DEPTH_FLAG) ? GL_DEPTH_BUFFER_BIT : 0;
    clear_bits |= (flags & CLEAR_STENCIL_FLAG) ? GL_STENCIL_BUFFER_BIT : 0;
    glClear(clear_bits);
}

void OGLBackend::lock_color_buffer() { glDrawBuffer(0); }

void OGLBackend::set_seamless_cubemaps_enabled(bool value)
{
    if(value)
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    else
        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void OGLBackend::set_depth_lock(bool value) { glDepthMask(!value); }

void OGLBackend::set_stencil_lock(bool value) { glStencilMask(!value); }

void OGLBackend::set_cull_mode(CullMode value)
{
    switch(value)
    {
    case CullMode::None:
        glDisable(GL_CULL_FACE);
        break;
    case CullMode::Front:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        break;
    case CullMode::Back:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        break;
    }
}

void OGLBackend::set_std_blending()
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OGLBackend::set_light_blending()
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
}

void OGLBackend::disable_blending() { glDisable(GL_BLEND); }

void OGLBackend::set_depth_func(DepthFunc value)
{
    switch(value)
    {
    case DepthFunc::Less:
        glDepthFunc(GL_LESS);
        break;

    case DepthFunc::LEqual:
        glDepthFunc(GL_LEQUAL);
        break;
    }
}

void OGLBackend::set_depth_test_enabled(bool value)
{
    if(value)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void OGLBackend::set_stencil_func(StencilFunc value, uint16_t a, uint16_t b)
{
    switch(value)
    {
    case StencilFunc::Always:
        glStencilFunc(GL_ALWAYS, 0, 0);
        break;

    case StencilFunc::NotEqual:
        glStencilFunc(GL_NOTEQUAL, a, b);
        break;
    }
}

void OGLBackend::set_stencil_operator(StencilOperator value)
{
    if(value == StencilOperator::LightVolume)
    {
#ifndef __OPTIM_LIGHT_VOLUMES_STENCIL_INVERT__
        // If depth test fails, back faces polygons increment the stencil value
        // else nothing changes
        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
        // If depth test fails, front faces polygons decrement the stencil value
        // else nothing changes
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
#else
        // If depth test fails, front and back polygons invert the stencil value bitwise
        // else nothing changes
        glStencilOp(GL_KEEP, GL_INVERT, GL_KEEP);
#endif //__OPTIM_LIGHT_VOLUMES_STENCIL_INVERT__
    }
}

void OGLBackend::set_stencil_test_enabled(bool value)
{
    if(value)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
}

void OGLBackend::finish() { glFinish(); }

void OGLBackend::flush() { glFlush(); }

static const std::map<GLenum, std::string> s_gl_errors = {{GL_NO_ERROR, "No error"},
                                                          {GL_INVALID_OPERATION, "Invalid operation"},
                                                          {GL_INVALID_ENUM, "Invalid enum"},
                                                          {GL_INVALID_VALUE, "Invalid value"},
                                                          {GL_STACK_OVERFLOW, "Stack overflow"},
                                                          {GL_STACK_UNDERFLOW, "Stack underflow"},
                                                          {GL_OUT_OF_MEMORY, "Out of memory"}};

static const std::string s_gl_unknown_error = "Unknown error";

const std::string& OGLBackend::show_error()
{
    auto it = s_gl_errors.find(glGetError());
    if(it != s_gl_errors.end())
        return it->second;
    else
        return s_gl_unknown_error;
}

uint32_t OGLBackend::get_error() { return glGetError(); }

void OGLBackend::assert_no_error() { K_ASSERT(glGetError() == 0, "OpenGL error occurred!"); }

void OGLBackend::set_pack_alignment(uint32_t value)
{
    K_ASSERT(is_power_of_2(value), "OGLBackend::set_pack_alignment: arg must be a power of 2.");
    glPixelStorei(GL_PACK_ALIGNMENT, value);
}

void OGLBackend::set_unpack_alignment(uint32_t value)
{
    K_ASSERT(is_power_of_2(value), "OGLBackend::set_pack_alignment: arg must be a power of 2.");
    glPixelStorei(GL_UNPACK_ALIGNMENT, value);
}

void OGLBackend::set_line_width(float value) { glLineWidth(value); }

/*
          _____  _                 _       _
         |  __ \(_)               | |     | |
         | |  | |_ ___ _ __   __ _| |_ ___| |__
         | |  | | / __| '_ \ / _` | __/ __| '_ \
         | |__| | \__ \ |_) | (_| | || (__| | | |
         |_____/|_|___/ .__/ \__,_|\__\___|_| |_|
                      | |
                      |_|
*/

namespace render_dispatch
{

void create_index_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    IndexBufferHandle handle;
    uint32_t count;
    DrawPrimitive primitive;
    UsagePattern mode;
    uint32_t* auxiliary;

    buf.read(&handle);
    buf.read(&count);
    buf.read(&primitive);
    buf.read(&mode);
    buf.read(&auxiliary);

    // Unbind currently bound VAO to avoid leaking state of newly created IBO
    GLint current_vao;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
    glBindVertexArray(0);

    s_storage.index_buffers[handle.index()].init(auxiliary, count, primitive, mode);

    // Restore VAO
    glBindVertexArray(current_vao);

    GL_END_DBG()
}

void create_vertex_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    VertexBufferHandle handle;
    VertexBufferLayoutHandle layout_hnd;
    uint32_t count;
    UsagePattern mode;
    float* auxiliary;
    buf.read(&handle);
    buf.read(&layout_hnd);
    buf.read(&count);
    buf.read(&mode);
    buf.read(&auxiliary);

    // Unbind currently bound VAO to avoid leaking state of newly created VBO
    GLint current_vao;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
    glBindVertexArray(0);

    const auto& layout = *s_storage.vertex_buffer_layouts[layout_hnd.index()];
    s_storage.vertex_buffers[handle.index()].init(auxiliary, count, layout, mode);

    // Restore VAO
    glBindVertexArray(current_vao);

    GL_END_DBG()
}

void create_vertex_array(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    VertexArrayHandle handle;
    IndexBufferHandle ib;
    VertexBufferHandle vb;
    buf.read(&handle);
    buf.read(&ib);
    buf.read(&vb);

    s_storage.vertex_arrays[handle.index()].init();
    s_storage.vertex_arrays[handle.index()].set_vertex_buffer(s_storage.vertex_buffers[vb.index()]);
    s_storage.vertex_array_dependencies[handle.index()].vbos.push_back(vb);
    if(!ib.is_null())
    {
        s_storage.vertex_arrays[handle.index()].set_index_buffer(s_storage.index_buffers[ib.index()]);
        s_storage.vertex_array_dependencies[handle.index()].ibo = ib;
    }


    // VAO binding state has changed, invalidate last bound VAO
    s_storage.invalidate_VAO_cache();

    GL_END_DBG()
}

void create_vertex_array_multiple_VBO(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    VertexArrayHandle handle;
    IndexBufferHandle ib;
    uint8_t VBO_count;
    buf.read(&handle);
    buf.read(&ib);
    buf.read(&VBO_count);

    s_storage.vertex_arrays[handle.index()].init();

    for(uint8_t ii = 0; ii < VBO_count; ++ii)
    {
        VertexBufferHandle vb;
        buf.read(&vb);
        s_storage.vertex_arrays[handle.index()].add_vertex_buffer(s_storage.vertex_buffers[vb.index()]);
        s_storage.vertex_array_dependencies[handle.index()].vbos.push_back(vb);
    }

    if(!ib.is_null())
    {
        s_storage.vertex_arrays[handle.index()].set_index_buffer(s_storage.index_buffers[ib.index()]);
        s_storage.vertex_array_dependencies[handle.index()].ibo = ib;
    }

    // VAO binding state has changed, invalidate last bound VAO
    s_storage.invalidate_VAO_cache();

    GL_END_DBG()
}

void create_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    UniformBufferHandle handle;
    uint32_t size;
    UsagePattern mode;
    std::string name;
    uint8_t* auxiliary;
    buf.read(&handle);
    buf.read(&size);
    buf.read(&mode);
    buf.read_str(name);
    buf.read(&auxiliary);

    s_storage.uniform_buffers[handle.index()].init(name, auxiliary, size, mode);
    GL_END_DBG()
}

void create_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    ShaderStorageBufferHandle handle;
    uint32_t size;
    UsagePattern mode;
    std::string name;
    uint8_t* auxiliary;
    buf.read(&handle);
    buf.read(&size);
    buf.read(&mode);
    buf.read_str(name);
    buf.read(&auxiliary);

    s_storage.shader_storage_buffers[handle.index()].init(name, auxiliary, size, mode);
    GL_END_DBG()
}

void create_shader(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    ShaderHandle handle;
    std::string filepath;
    std::string name;
    buf.read(&handle);
    buf.read_str(filepath);
    buf.read_str(name);

    auto ref = make_ref<OGLShader>();
    ref->init(name, std::string(filepath));
    s_storage.shaders[handle.index()] = ref;
    // s_storage.shader_compat[handle.index].set_layout(s_storage.shaders[handle.index]->get_attribute_layout());

    // Shader state has changed, invalidate last bound shader
    s_storage.invalidate_shader_cache();

    GL_END_DBG()
}

void create_texture_2D(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    TextureHandle handle;
    Texture2DDescriptor descriptor;
    buf.read(&handle);
    buf.read(&descriptor);

    s_storage.textures[handle.index()].init(descriptor);
    // Free resources if needed
    descriptor.release();

    // Texture state has changed, invalidate last bound textures
    s_storage.invalidate_texture_cache();

    GL_END_DBG()
}

void create_cubemap(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    CubemapHandle handle;
    CubemapDescriptor descriptor;
    buf.read(&handle);
    buf.read(&descriptor);

    s_storage.cubemaps[handle.index()].init(descriptor);

    // Cubemap state has changed, invalidate last bound cubemaps
    s_storage.invalidate_cubemap_cache();

    GL_END_DBG()
}

void create_framebuffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    uint32_t width;
    uint32_t height;
    uint32_t count;
    uint8_t flags;
    FramebufferHandle handle;
    FramebufferLayoutElement* auxiliary;
    buf.read(&handle);
    buf.read(&width);
    buf.read(&height);
    buf.read(&flags);
    buf.read(&count);
    buf.read(&auxiliary);

    FramebufferLayout layout(auxiliary, count);
    const auto& texture_vector = s_storage.framebuffer_textures_[handle.index()];
    s_storage.framebuffers[handle.index()] = make_scope<OGLFramebuffer>(width, height, flags, layout, texture_vector);
    GL_END_DBG()
}

void update_index_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    IndexBufferHandle handle;
    uint32_t count;
    uint32_t* auxiliary;
    buf.read(&handle);
    buf.read(&count);
    buf.read(&auxiliary);

    s_storage.index_buffers[handle.index()].map(auxiliary, count * sizeof(uint32_t));
    GL_END_DBG()
}

void update_vertex_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    VertexBufferHandle handle;
    uint32_t size;
    uint8_t* auxiliary;
    buf.read(&handle);
    buf.read(&size);
    buf.read(&auxiliary);

    s_storage.vertex_buffers[handle.index()].map(auxiliary, size);
    GL_END_DBG()
}

void update_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    UniformBufferHandle handle;
    uint32_t size;
    uint8_t* auxiliary;
    buf.read(&handle);
    buf.read(&size);
    buf.read(&auxiliary);

    auto& UBO = s_storage.uniform_buffers[handle.index()];
    UBO.map(auxiliary, size ? size : UBO.get_size());
    GL_END_DBG()
}

void update_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    ShaderStorageBufferHandle handle;
    uint32_t size;
    uint8_t* auxiliary;
    buf.read(&handle);
    buf.read(&size);
    buf.read(&auxiliary);

    s_storage.shader_storage_buffers[handle.index()].map(auxiliary, size);
    GL_END_DBG()
}

void shader_attach_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    ShaderHandle shader_handle;
    UniformBufferHandle ubo_handle;
    buf.read(&shader_handle);
    buf.read(&ubo_handle);

    auto& shader = *s_storage.shaders[shader_handle.index()];
    shader.attach_uniform_buffer(s_storage.uniform_buffers[ubo_handle.index()]);
    GL_END_DBG()
}

void shader_attach_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    ShaderHandle shader_handle;
    ShaderStorageBufferHandle ssbo_handle;
    buf.read(&shader_handle);
    buf.read(&ssbo_handle);

    auto& shader = *s_storage.shaders[shader_handle.index()];
    shader.attach_shader_storage(s_storage.shader_storage_buffers[ssbo_handle.index()]);
    GL_END_DBG()
}

void update_framebuffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    FramebufferHandle fb_handle;
    uint32_t width;
    uint32_t height;

    buf.read(&fb_handle);
    buf.read(&width);
    buf.read(&height);

    uint8_t flags = s_storage.framebuffers[fb_handle.index()]->get_flags();
    auto layout = s_storage.framebuffers[fb_handle.index()]->get_layout();
    auto& texture_vector = s_storage.framebuffer_textures_[fb_handle.index()];
    s_storage.framebuffers[fb_handle.index()] = make_scope<OGLFramebuffer>(width, height, flags, layout, texture_vector);
    GL_END_DBG()
}

void clear_framebuffers(memory::LinearBuffer<>&)
{
    K_ASSERT(false, "Clear framebuffers not implemented.");
    /*FramebufferPool::traverse_framebuffers([](FramebufferHandle handle)
    {
        s_storage.framebuffers[handle.index]->bind();
        gfx::backend->clear(ClearFlags::CLEAR_COLOR_FLAG | ClearFlags::CLEAR_DEPTH_FLAG);
    });*/
}

void set_host_window_size(memory::LinearBuffer<>& buf)
{
    uint32_t width;
    uint32_t height;
    buf.read(&width);
    buf.read(&height);

    s_storage.host_window_size_ = {width, height};
}

void nop(memory::LinearBuffer<>&) {}

void get_pixel_data(memory::LinearBuffer<>& buf)
{
    GL_BEGIN_DBG()
    TextureHandle handle;
    size_t promise_token;

    buf.read(&handle);
    buf.read(&promise_token);

    auto&& [data, size] = s_storage.textures[handle.index()].read_pixels();
    s_storage.texture_data_promises_.fulfill(promise_token, PixelData{data, size});
    GL_END_DBG()
}

void generate_cubemap_mipmaps(memory::LinearBuffer<>& buf)
{
    GL_BEGIN_DBG()
    CubemapHandle handle;
    buf.read(&handle);

    s_storage.cubemaps[handle.index()].generate_mipmaps();
    GL_END_DBG()
}

void framebuffer_screenshot(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    FramebufferHandle handle;
    std::string filepath;
    buf.read(&handle);
    buf.read_str(filepath);

    s_storage.framebuffers[handle.index()]->screenshot(filepath);
    GL_END_DBG()
}

void destroy_index_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    IndexBufferHandle handle;
    buf.read(&handle);
    s_storage.index_buffers[handle.index()].release();
    handle.release();
    GL_END_DBG()
}

void destroy_vertex_buffer_layout(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    VertexBufferLayoutHandle handle;
    buf.read(&handle);
    s_storage.vertex_buffer_layouts[handle.index()] = nullptr;
    handle.release();
    GL_END_DBG()
}

void destroy_vertex_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    VertexBufferHandle handle;
    buf.read(&handle);
    s_storage.vertex_buffers[handle.index()].release();
    handle.release();
    GL_END_DBG()
}

void destroy_vertex_array(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    VertexArrayHandle handle;
    buf.read(&handle);
    s_storage.vertex_arrays[handle.index()].release();
    s_storage.vertex_array_dependencies[handle.index()].release(); // Release VBOs/IBO handles
    handle.release();
    GL_END_DBG()
}

void destroy_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    UniformBufferHandle handle;
    buf.read(&handle);
    s_storage.uniform_buffers[handle.index()].release();
    handle.release();
    GL_END_DBG()
}

void destroy_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    ShaderStorageBufferHandle handle;
    buf.read(&handle);
    s_storage.shader_storage_buffers[handle.index()].release();
    handle.release();
    GL_END_DBG()
}

void destroy_shader(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    ShaderHandle handle;
    buf.read(&handle);
    s_storage.shaders[handle.index()] = nullptr;
    // s_storage.shader_compat[handle.index].clear();
    handle.release();
    GL_END_DBG()
}

void destroy_texture_2D(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    TextureHandle handle;
    buf.read(&handle);
    s_storage.textures[handle.index()].release();
    handle.release();
    GL_END_DBG()
}

void destroy_cubemap(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    CubemapHandle handle;
    buf.read(&handle);
    s_storage.cubemaps[handle.index()].release();
    handle.release();
    GL_END_DBG()
}

void destroy_framebuffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    FramebufferHandle handle;
    bool detach_textures;
    buf.read(&handle);
    buf.read(&detach_textures);

    bool has_cubemap = s_storage.framebuffers[handle.index()]->has_cubemap();
    s_storage.framebuffers[handle.index()] = nullptr;

    // Delete framebuffer textures if they are not detached
    if(!detach_textures)
    {
        auto& texture_vector = s_storage.framebuffer_textures_[handle.index()];
        if(!has_cubemap)
        {
            for(uint32_t ii = 0; ii < texture_vector.handles.size(); ++ii)
            {
                uint16_t tex_index = texture_vector.handles[ii].index();
                s_storage.textures[tex_index].release();
                texture_vector.handles[ii].release();
            }
        }
        else
        {
            uint16_t cm_index = texture_vector.cubemap.index();
            s_storage.cubemaps[cm_index].release();
            texture_vector.cubemap.release();
        }
    }

    s_storage.framebuffer_textures_.erase(handle.index());
    handle.release();
    GL_END_DBG()
}

} // namespace render_dispatch

// Helper function to identify which part of the pass state has changed
static inline bool has_mutated(uint64_t state, uint64_t old_state, uint64_t mask)
{
    if constexpr (k_enable_state_cache)
        return ((state ^ old_state) & mask) > 0;
    else
    {
        (void)state;
        (void)old_state;
        (void)mask;
        return true;
    }
}

static void handle_state(uint64_t state_flags)
{
    // * If pass state has changed, decode it, find which parts have changed and update device state
    if(state_flags != s_storage.state_cache_)
    {
        RenderState state;
        state.decode(state_flags);

        if(has_mutated(state_flags, s_storage.state_cache_, k_framebuffer_mask) ||
           has_mutated(state_flags, s_storage.state_cache_, k_target_mips_mask))
        {
            if(state.render_target == s_storage.default_framebuffer_.index())
            {
                gfx::backend->bind_default_framebuffer();
                gfx::backend->viewport(0, 0, s_storage.host_window_size_.x, s_storage.host_window_size_.y);
            }
            else
                s_storage.framebuffers[state.render_target]->bind(state.target_mip_level);

            s_storage.current_framebuffer_index_ = state.render_target;

            // Only clear on render target switch, if clear flags are set
            if(state.rasterizer_state.clear_flags != ClearFlags::CLEAR_NONE)
                gfx::backend->clear(state.rasterizer_state.clear_flags);
        }

        if(has_mutated(state_flags, s_storage.state_cache_, k_cull_mode_mask))
            gfx::backend->set_cull_mode(state.rasterizer_state.cull_mode);

        if(has_mutated(state_flags, s_storage.state_cache_, k_transp_mask))
        {
            switch(state.blend_state)
            {
            case BlendState::Alpha:
                gfx::backend->set_std_blending();
                break;
            case BlendState::Light:
                gfx::backend->set_light_blending();
                break;
            default:
                gfx::backend->disable_blending();
                break;
            }
        }

        if(has_mutated(state_flags, s_storage.state_cache_, k_stencil_test_mask))
        {
            gfx::backend->set_stencil_test_enabled(state.depth_stencil_state.stencil_test_enabled);
            if(state.depth_stencil_state.stencil_test_enabled)
            {
                gfx::backend->set_stencil_func(state.depth_stencil_state.stencil_func);
                gfx::backend->set_stencil_operator(state.depth_stencil_state.stencil_operator);
            }
        }

        if(has_mutated(state_flags, s_storage.state_cache_, k_depth_test_mask))
        {
            gfx::backend->set_depth_test_enabled(state.depth_stencil_state.depth_test_enabled);
            if(state.depth_stencil_state.depth_test_enabled)
                gfx::backend->set_depth_func(state.depth_stencil_state.depth_func);
        }

        if(has_mutated(state_flags, s_storage.state_cache_, k_depth_lock_mask))
            gfx::backend->set_depth_lock(state.depth_stencil_state.depth_lock);

        if(has_mutated(state_flags, s_storage.state_cache_, k_stencil_lock_mask))
            gfx::backend->set_stencil_lock(state.depth_stencil_state.stencil_lock);

        s_storage.state_cache_ = state_flags;
    }
}

namespace draw_dispatch
{

void draw(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    DrawCall::DrawCallType type;
    DrawCall::Data data;
    buf.read(&type);
    buf.read(&data);

    handle_state(data.state_flags);

    // * Detect if a new shader needs to be used, update and bind shader resources
    auto& shader = *s_storage.shaders[data.shader.index()];

    if constexpr (k_enable_state_cache)
    {
        if(data.shader.index() != s_storage.last_shader_index)
        {
            shader.bind();
            s_storage.last_shader_index = data.shader.index();
            s_storage.invalidate_texture_cache();
            s_storage.invalidate_cubemap_cache();
        }
    }
    else
        shader.bind();

    uint8_t texture_count;
    buf.read(&texture_count);
    for(uint8_t ii = 0; ii < texture_count; ++ii)
    {
        TextureHandle hnd;
        buf.read(&hnd);

        if(hnd.is_null())
            continue;

        // Avoid texture switching if not necessary
        if constexpr (k_enable_state_cache)
        {
            if(hnd.data != s_storage.last_texture_index[ii])
            {
                const auto& texture = s_storage.textures[hnd.index()];
                shader.attach_texture_2D(texture, ii);
                s_storage.last_texture_index[ii] = hnd.index();
            }
        }
        else
        {
            const auto& texture = s_storage.textures[hnd.index()];
            shader.attach_texture_2D(texture, ii);
        }
    }

    uint8_t cubemap_count;
    buf.read(&cubemap_count);
    for(uint8_t ii = 0; ii < cubemap_count; ++ii)
    {
        CubemapHandle hnd;
        buf.read(&hnd);

        // Avoid texture switching if not necessary
        if constexpr (k_enable_state_cache)
        {
            if(hnd.index() != s_storage.last_cubemap_index[ii])
            {
                const auto& cubemap = s_storage.cubemaps[hnd.index()];
                shader.attach_cubemap(cubemap, ii + texture_count); // Cubemap samplers after 2d samplers (this is awkward)
                s_storage.last_cubemap_index[ii] = hnd.index();
            }
        }
        else
        {
            const auto& cubemap = s_storage.cubemaps[hnd.index()];
            shader.attach_cubemap(cubemap, ii + texture_count); // Cubemap samplers after 2d samplers (this is awkward)
        }
    }

    // * Execute draw call
    auto& va = s_storage.vertex_arrays[data.VAO.index()];
    // Avoid switching vertex array when possible
    if constexpr (k_enable_state_cache)
    {
        if(data.VAO.data != s_storage.last_VAO_index)
        {
            va.bind();
            s_storage.last_VAO_index = data.VAO.index();
        }
    }
    else
        va.bind();

    switch(type)
    {
    case DrawCall::Indexed:
        glDrawElements(OGLPrimitive.at(va.get_index_buffer().get_primitive()),
                       (bool(data.count) ? data.count : va.get_index_buffer().get_count()), GL_UNSIGNED_INT,
                       reinterpret_cast<void*>(data.offset * sizeof(GLuint)));
        break;
    case DrawCall::Array:
        glDrawArrays(OGLPrimitive.at(va.get_index_buffer().get_primitive()), data.offset,
                     (bool(data.count) ? data.count : va.get_vertex_buffer().get_count()));
        break;
    case DrawCall::IndexedInstanced: {
        // Read additional data needed for instanced rendering
        uint32_t instance_count;
        buf.read(&instance_count);
        // ASSUME SSBO is attached to shader, so it is already bound at this stage
        glDrawElementsInstanced(OGLPrimitive.at(va.get_index_buffer().get_primitive()),
                                (bool(data.count) ? data.count : va.get_index_buffer().get_count()), GL_UNSIGNED_INT,
                                reinterpret_cast<void*>(data.offset * sizeof(GLuint)), instance_count);
        break;
    }
    default:
        K_ASSERT(false, "Specified draw call type is unsupported at the moment.");
        break;
    }

    GL_END_DBG()
}

void clear(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    FramebufferHandle target;
    uint32_t flags;
    kb::math::argb32_t clear_color;
    buf.read(&target);
    buf.read(&flags);
    buf.read(&clear_color);

    auto color = kb::math::ColorRGBA(clear_color);

    gfx::backend->set_clear_color(color.r, color.g, color.b, color.a);
    if(target == s_storage.default_framebuffer_ && flags != ClearFlags::CLEAR_NONE)
    {
        gfx::backend->bind_default_framebuffer();
        gfx::backend->viewport(0, 0, s_storage.host_window_size_.x, s_storage.host_window_size_.y);
        gfx::backend->clear(int(flags));
    }
    else
    {
        auto& fb = *s_storage.framebuffers[target.index()];
        fb.bind();
        gfx::backend->viewport(0, 0, float(fb.get_width()), float(fb.get_height()));
        gfx::backend->clear(int(flags));
    }

    if(s_storage.current_framebuffer_index_ != target.index())
    {
        // Rebind current framebuffer
        if(s_storage.current_framebuffer_index_ == s_storage.default_framebuffer_.index())
        {
            gfx::backend->bind_default_framebuffer();
            gfx::backend->viewport(0, 0, s_storage.host_window_size_.x, s_storage.host_window_size_.y);
        }
        else
        {
            auto& fb = *s_storage.framebuffers[s_storage.current_framebuffer_index_];
            fb.bind();
            gfx::backend->viewport(0, 0, float(fb.get_width()), float(fb.get_height()));
        }
    }
    gfx::backend->set_clear_color(0.f, 0.f, 0.f, 0.f);
    GL_END_DBG()
}

void blit_depth(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    FramebufferHandle source;
    FramebufferHandle target;
    buf.read(&source);
    buf.read(&target);

    s_storage.framebuffers[target.index()]->blit_depth(*s_storage.framebuffers[source.index()]);
    GL_END_DBG()
}

void update_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()

    ShaderStorageBufferHandle ssbo_handle;
    uint32_t size;
    void* data;

    buf.read(&ssbo_handle);
    buf.read(&size);
    buf.read(&data);

    s_storage.shader_storage_buffers[ssbo_handle.index()].stream(data, size, 0);
    /*
        auto& ssbo = *s_storage.shader_storage_buffers[ssbo_handle.index];
        if(!ssbo.has_persistent_mapping())
            ssbo.stream(data, size, 0);
        else
            ssbo.map_persistent(data, size, 0);
    */
    GL_END_DBG()
}

void update_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()
    GL_BEGIN_DBG()
    
    UniformBufferHandle ubo_handle;
    uint32_t size;
    void* data;

    buf.read(&ubo_handle);
    buf.read(&size);
    buf.read(&data);

    auto& ubo = s_storage.uniform_buffers[ubo_handle.index()];
    ubo.stream(data, size ? size : ubo.get_size(), 0);
    GL_END_DBG()
}

} // namespace draw_dispatch

typedef void (*backend_dispatch_func_t)(memory::LinearBuffer<>&);
static backend_dispatch_func_t render_backend_dispatch[std::size_t(RenderCommand::Count)] = {
    &render_dispatch::create_index_buffer,
    &render_dispatch::create_vertex_buffer,
    &render_dispatch::create_vertex_array,
    &render_dispatch::create_vertex_array_multiple_VBO,
    &render_dispatch::create_uniform_buffer,
    &render_dispatch::create_shader_storage_buffer,
    &render_dispatch::create_shader,
    &render_dispatch::create_texture_2D,
    &render_dispatch::create_cubemap,
    &render_dispatch::create_framebuffer,
    &render_dispatch::update_index_buffer,
    &render_dispatch::update_vertex_buffer,
    &render_dispatch::update_uniform_buffer,
    &render_dispatch::update_shader_storage_buffer,
    &render_dispatch::shader_attach_uniform_buffer,
    &render_dispatch::shader_attach_storage_buffer,
    &render_dispatch::update_framebuffer,
    &render_dispatch::clear_framebuffers,
    &render_dispatch::set_host_window_size,

    &render_dispatch::nop,

    &render_dispatch::get_pixel_data,
    &render_dispatch::generate_cubemap_mipmaps,
    &render_dispatch::framebuffer_screenshot,
    &render_dispatch::destroy_index_buffer,
    &render_dispatch::destroy_vertex_buffer_layout,
    &render_dispatch::destroy_vertex_buffer,
    &render_dispatch::destroy_vertex_array,
    &render_dispatch::destroy_uniform_buffer,
    &render_dispatch::destroy_shader_storage_buffer,
    &render_dispatch::destroy_shader,
    &render_dispatch::destroy_texture_2D,
    &render_dispatch::destroy_cubemap,
    &render_dispatch::destroy_framebuffer,
};

static backend_dispatch_func_t draw_backend_dispatch[std::size_t(RenderCommand::Count)] = {
    &draw_dispatch::draw,
    &draw_dispatch::clear,
    &draw_dispatch::blit_depth,
    &draw_dispatch::update_shader_storage_buffer,
    &draw_dispatch::update_uniform_buffer,
};

void OGLBackend::dispatch_command(uint16_t type, memory::LinearBuffer<>& buf) { (*render_backend_dispatch[type])(buf); }

void OGLBackend::dispatch_draw(uint16_t type, memory::LinearBuffer<>& buf) { (*draw_backend_dispatch[type])(buf); }

} // namespace erwin