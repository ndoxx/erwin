#include "render/buffer_layout.h"
#include "debug/logger.h"

#include <cstring>

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

bool BufferLayoutElement::operator ==(const BufferLayoutElement& other)
{
    return name == other.name
        && type == other.type
        && normalized == other.normalized;
}


BufferLayout::BufferLayout(const std::initializer_list<BufferLayoutElement>& elements):
elements_(elements),
stride_(0)
{
    compute_offset_and_stride();
}

BufferLayout::BufferLayout(BufferLayoutElement* elements, uint32_t count):
stride_(0)
{
    init(elements, count);
}

void BufferLayout::init(BufferLayoutElement* elements, uint32_t count)
{
    elements_.resize(count);
    memcpy(elements_.data(), elements, count * sizeof(BufferLayoutElement));
    compute_offset_and_stride();
}

void BufferLayout::clear()
{
    stride_ = 0;
    elements_.clear();
}

bool BufferLayout::operator ==(const BufferLayout& other)
{
    if(elements_.size() != other.elements_.size())
        return false;

    bool ret = true;
    for(size_t ii=0; ii<elements_.size(); ++ii)
        ret &= elements_[ii] == other.elements_[ii];
    return ret;
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

} // namespace erwin