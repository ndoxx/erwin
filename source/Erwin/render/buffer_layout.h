#pragma once

#include <vector>
#include "render/render_state.h"

namespace erwin
{

// Represents a vertex attribute
struct BufferLayoutElement
{
	BufferLayoutElement() = default;
    BufferLayoutElement(hash_t name, ShaderDataType type, bool normalized=false);

    // Get the number of components for the underlying type
    uint32_t get_component_count() const;

    hash_t name;
    ShaderDataType type;
    uint32_t size;
    uint32_t offset;
    bool normalized;
};

// Represents the data structure inside a vertex buffer
class BufferLayout
{
public:
    BufferLayout(const std::initializer_list<BufferLayoutElement>& elements);
    BufferLayout(BufferLayoutElement* elements, uint32_t count);

    // Iterators for use in range-based for loops
    inline std::vector<BufferLayoutElement>::iterator begin()             { return elements_.begin(); }
    inline std::vector<BufferLayoutElement>::iterator end()               { return elements_.end(); }
    inline std::vector<BufferLayoutElement>::const_iterator begin() const { return elements_.begin(); }
    inline std::vector<BufferLayoutElement>::const_iterator end() const   { return elements_.end(); }

    inline uint32_t get_stride() const { return stride_; }
    inline size_t get_count() const    { return elements_.size(); }

private:
	// Helper function to set each element's offset and calculate buffer stride
    void compute_offset_and_stride();

private:
    std::vector<BufferLayoutElement> elements_;
    uint32_t stride_;
};

/*
	Example layout:

	BufferLayout VertexAnimLayout =
	{
	    {"a_position"_h, ShaderDataType::Vec3},
	    {"a_normal"_h,   ShaderDataType::Vec3},
	    {"a_tangent"_h,  ShaderDataType::Vec3},
	    {"a_texCoord"_h, ShaderDataType::Vec2},
	    {"a_weights"_h,  ShaderDataType::Vec4},
	    {"a_boneIDs"_h,  ShaderDataType::IVec4},
	};
*/


} // namespace erwin