#include "render/buffer.h"
#include "debug/logger.h"

#include "render/render_device.h"
#include "platform/ogl_buffer.h"

namespace erwin
{

WRef<VertexBuffer> VertexBuffer::create(float* vertex_data, uint32_t count, const BufferLayout& layout, DrawMode mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "VertexBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_ref<OGLVertexBuffer>(vertex_data, count, layout, mode);
    }
}

WRef<IndexBuffer> IndexBuffer::create(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "IndexBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_ref<OGLIndexBuffer>(index_data, count, primitive, mode);
    }
}

WRef<VertexArray> VertexArray::create()
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "VertexArray: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_ref<OGLVertexArray>();
    }
}

WRef<UniformBuffer> UniformBuffer::create(const std::string& name, void* data, uint32_t struct_size, DrawMode mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "UniformBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_ref<OGLUniformBuffer>(name, data, struct_size, mode);
    }
}

WRef<ShaderStorageBuffer> ShaderStorageBuffer::create(const std::string& name, void* data, uint32_t size, DrawMode mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "ShaderStorageBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_ref<OGLShaderStorageBuffer>(name, data, size, mode);
    }
}


} // namespace erwin