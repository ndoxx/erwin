#include "render/buffer.h"
#include "debug/logger.h"

#include "render/render_device.h"
#include "platform/ogl_buffer.h"

namespace erwin
{

WRef<VertexBuffer> VertexBuffer::create(float* vertex_data, uint32_t count, const BufferLayout& layout, UsagePattern mode)
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

VertexBuffer* VertexBuffer::create(PoolArena& pool, float* vertex_data, uint32_t count, const BufferLayout& layout, UsagePattern mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "VertexBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return W_NEW(OGLVertexBuffer, pool)(vertex_data, count, layout, mode);
    }
}

size_t VertexBuffer::node_size()
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:   return 0;
        case GfxAPI::OpenGL: return sizeof(OGLVertexBuffer);
    }
}



WRef<IndexBuffer> IndexBuffer::create(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, UsagePattern mode)
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

IndexBuffer* IndexBuffer::create(PoolArena& pool, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, UsagePattern mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "IndexBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return W_NEW(OGLIndexBuffer, pool)(index_data, count, primitive, mode);
    }
}

size_t IndexBuffer::node_size()
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:   return 0;
        case GfxAPI::OpenGL: return sizeof(OGLIndexBuffer);
    }
}



WRef<UniformBuffer> UniformBuffer::create(const std::string& name, void* data, uint32_t struct_size, UsagePattern mode)
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

UniformBuffer* UniformBuffer::create(PoolArena& pool, const std::string& name, void* data, uint32_t struct_size, UsagePattern mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "UniformBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return W_NEW(OGLUniformBuffer, pool)(name, data, struct_size, mode);
    }
}

size_t UniformBuffer::node_size()
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:   return 0;
        case GfxAPI::OpenGL: return sizeof(OGLUniformBuffer);
    }
}



WRef<ShaderStorageBuffer> ShaderStorageBuffer::create(const std::string& name, void* data, uint32_t size, UsagePattern mode)
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

ShaderStorageBuffer* ShaderStorageBuffer::create(PoolArena& pool, const std::string& name, void* data, uint32_t size, UsagePattern mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "ShaderStorageBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return W_NEW(OGLShaderStorageBuffer, pool)(name, data, size, mode);
    }
}

size_t ShaderStorageBuffer::node_size()
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:   return 0;
        case GfxAPI::OpenGL: return sizeof(OGLShaderStorageBuffer);
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

VertexArray* VertexArray::create(PoolArena& pool)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "VertexArray: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return W_NEW(OGLVertexArray, pool)();
    }
}

size_t VertexArray::node_size()
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:   return 0;
        case GfxAPI::OpenGL: return sizeof(OGLShaderStorageBuffer);
    }
}


} // namespace erwin