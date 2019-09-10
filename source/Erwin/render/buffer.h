#pragma once

#include <memory>
#include <vector>

#include "core/wtypes.h"

namespace erwin
{

// Describes data types held in buffer layouts and passed to shaders
enum class ShaderDataType: uint8_t
{
    Float = 0, Vec2, Vec3, Vec4, Mat3, Mat4, Int, IVec2, IVec3, IVec4, Bool
};

// Represents a vertex attribute
struct BufferLayoutElement
{
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

class VertexBuffer
{
public:
    VertexBuffer(const BufferLayout& layout, std::size_t size): layout_(layout), size_(size) {}
    virtual ~VertexBuffer() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual void stream(float* vertex_data, std::size_t size, std::size_t offset) const = 0;

    // Return the buffer layout
	inline const BufferLayout& get_layout() const { return layout_; }
    // Return the size (in bytes) of this vertex buffer
    inline std::size_t get_count() const { return size_; }

    // Factory method to create the correct implementation
    // for the current renderer API
    static VertexBuffer* create(float* vertex_data, std::size_t size, const BufferLayout& layout, bool dynamic=false);

protected:
    BufferLayout layout_;
    std::size_t size_;
};

class IndexBuffer
{
public:
    IndexBuffer(uint32_t count): count_(count) {}
    virtual ~IndexBuffer() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual void stream(uint32_t* index_data, uint32_t count, std::size_t offset) const = 0;

    // Return the number of indices
    inline uint32_t get_count() const { return count_; }
    // Return the size (in bytes) of this buffer
    inline uint32_t get_size() const  { return count_*sizeof(uint32_t); }

    // Factory method to create the correct implementation
    // for the current renderer API
    static IndexBuffer* create(uint32_t* index_data, uint32_t count, bool dynamic=false);

protected:
    uint32_t count_;
};

class VertexArray
{
public:
    VertexArray() {}
    virtual ~VertexArray() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual void add_vertex_buffer(std::shared_ptr<VertexBuffer> p_vb) = 0;
    virtual void set_index_buffer(std::shared_ptr<IndexBuffer> p_ib) = 0;
    
    inline const std::vector<std::shared_ptr<VertexBuffer>>& get_vertex_buffers() const { return vertex_buffers_; }
    inline const IndexBuffer& get_index_buffer() const { return *index_buffer_; }

    // Factory method to create the correct implementation
    // for the current renderer API
    static VertexArray* create();

protected:
	std::vector<std::shared_ptr<VertexBuffer>> vertex_buffers_;
	std::shared_ptr<IndexBuffer> index_buffer_ = nullptr;
};

} // namespace erwin