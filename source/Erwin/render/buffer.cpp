#include "render/buffer.h"
#include "debug/logger.h"

#include "render/render_device.h"
#include "platform/ogl_buffer.h"

namespace erwin
{

// Return size (in bytes) of specified data type
static uint32_t data_type_to_size(ShaderDataType type)
{
    switch(type)
    {
        case ShaderDataType::Float: return sizeof(float);
        case ShaderDataType::Vec2:  return sizeof(float) * 2;
        case ShaderDataType::Vec3:  return sizeof(float) * 3;
        case ShaderDataType::Vec4:  return sizeof(float) * 4;
        case ShaderDataType::Mat3:  return sizeof(float) * 3 * 3;
        case ShaderDataType::Mat4:  return sizeof(float) * 4 * 4;
        case ShaderDataType::Int:   return sizeof(int);
        case ShaderDataType::IVec2: return sizeof(int) * 2;
        case ShaderDataType::IVec3: return sizeof(int) * 3;
        case ShaderDataType::IVec4: return sizeof(int) * 4;
        case ShaderDataType::Bool:  return sizeof(bool);
    }

    DLOGE("render") << "Unknown ShaderDataType: " << int(type) << std::endl;
    return 0;
}

BufferLayoutElement::BufferLayoutElement(hash_t name, ShaderDataType type, bool normalized):
name(name),
type(type),
size(data_type_to_size(type)),
offset(0),
normalized(normalized)
{

}

uint32_t BufferLayoutElement::get_component_count() const
{
    switch(type)
    {
        case ShaderDataType::Float: return 1;
        case ShaderDataType::Vec2:  return 2;
        case ShaderDataType::Vec3:  return 3;
        case ShaderDataType::Vec4:  return 4;
        case ShaderDataType::Mat3:  return 3 * 3;
        case ShaderDataType::Mat4:  return 4 * 4;
        case ShaderDataType::Int:   return 1;
        case ShaderDataType::IVec2: return 2;
        case ShaderDataType::IVec3: return 3;
        case ShaderDataType::IVec4: return 4;
        case ShaderDataType::Bool:  return 1;
    }

    DLOGE("render") << "Unknown ShaderDataType: " << int(type) << std::endl;
    return 0;
}

BufferLayout::BufferLayout(const std::initializer_list<BufferLayoutElement>& elements):
elements_(elements),
stride_(0)
{
    compute_offset_and_stride();
}

void BufferLayout::compute_offset_and_stride()
{
	// Each element offset is determined by previous elements' sizes
	// Stride is the total size of each element
    uint32_t offset = 0;
    stride_ = 0;
    for(auto&& element: elements_)
    {
        element.offset = offset;
        offset += element.size;
        stride_ += element.size;
    }
}

VertexBuffer* VertexBuffer::create(float* vertex_data, uint32_t count, const BufferLayout& layout, DrawMode mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "VertexBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return new OGLVertexBuffer(vertex_data, count, layout, mode);
    }
}

IndexBuffer* IndexBuffer::create(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "IndexBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return new OGLIndexBuffer(index_data, count, primitive, mode);
    }
}

VertexArray* VertexArray::create()
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "VertexArray: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return new OGLVertexArray();
    }
}

ShaderStorageBuffer* ShaderStorageBuffer::create(void* data, uint32_t count, uint32_t struct_size, DrawMode mode)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "ShaderStorageBuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return new OGLShaderStorageBuffer(data, count, struct_size, mode);
    }
}


} // namespace erwin