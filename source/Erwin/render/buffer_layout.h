#pragma once

#include <vector>
#include <cstdint>

#include "core/core.h"

namespace erwin
{

// Describes data types held in buffer layouts and passed to shaders
enum class ShaderDataType: uint8_t
{
    Float = 0, Vec2, Vec3, Vec4, Mat3, Mat4, Int, IVec2, IVec3, IVec4, Bool
};

enum class UsagePattern: uint8_t
{
    Static = 0, Stream, Dynamic, PersistentMapping
};

enum class DrawPrimitive
{
    Lines = 2,
    Triangles = 3,
    Quads = 4
};

// Represents a vertex attribute
struct BufferLayoutElement
{
	BufferLayoutElement() = default;
    BufferLayoutElement(hash_t name, ShaderDataType type, bool normalized=false);

    // Get the number of components for the underlying type
    uint32_t get_component_count() const;

    bool operator ==(const BufferLayoutElement& other);

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
    BufferLayout() = default;
    explicit BufferLayout(const std::initializer_list<BufferLayoutElement>& elements);
    BufferLayout(BufferLayoutElement const* elements, uint32_t count);

    void init(BufferLayoutElement const* elements, uint32_t count);
    void clear();

    bool operator ==(const BufferLayout& other);
    bool compare(const BufferLayout& other);

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