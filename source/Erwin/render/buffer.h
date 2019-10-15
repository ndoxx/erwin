#pragma once

#include <memory>
#include <vector>

#include "core/core.h"
#include "core/wtypes.h"
#include "core/unique_id.h"

namespace erwin
{

// Describes data types held in buffer layouts and passed to shaders
enum class ShaderDataType: uint8_t
{
    Float = 0, Vec2, Vec3, Vec4, Mat3, Mat4, Int, IVec2, IVec3, IVec4, Bool
};

enum class DrawMode: uint8_t
{
    Static = 0, Stream, Dynamic
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
    VertexBuffer(const BufferLayout& layout, uint32_t count): unique_id_(id::unique_id()), layout_(layout), count_(count) {}
    virtual ~VertexBuffer() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual void stream(float* vertex_data, uint32_t count, std::size_t offset) = 0;
    virtual void map(float* vertex_data, uint32_t count) = 0;

    // Return the buffer layout
	inline const BufferLayout& get_layout() const { return layout_; }
    // Return the vertex count of this vertex buffer
    inline std::size_t get_count() const { return count_; }
    // Return the size (in bytes) of this vertex buffer
    inline std::size_t get_size() const  { return count_*sizeof(float); }
    // Return engine unique id for this object
    inline W_ID get_unique_id() const { return unique_id_; }

    // Factory method to create the correct implementation
    // for the current renderer API
    static WRef<VertexBuffer> create(float* vertex_data, uint32_t count, const BufferLayout& layout, DrawMode mode = DrawMode::Static);

protected:
    W_ID unique_id_;
    BufferLayout layout_;
    std::size_t count_;
};

enum class DrawPrimitive;
class IndexBuffer
{
public:
    IndexBuffer(uint32_t count, DrawPrimitive primitive): unique_id_(id::unique_id()), count_(count), primitive_(primitive) {}
    virtual ~IndexBuffer() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual void stream(uint32_t* index_data, uint32_t count, std::size_t offset) = 0;
    virtual void map(uint32_t* index_data, uint32_t count) = 0;

    // Return the number of indices
    inline uint32_t get_count() const { return count_; }
    // Return the size (in bytes) of this buffer
    inline uint32_t get_size() const  { return count_*sizeof(uint32_t); }
    // Return the intended draw primitive
    inline DrawPrimitive get_primitive() const { return primitive_; }
    // Return engine unique id for this object
    inline W_ID get_unique_id() const { return unique_id_; }

    // Factory method to create the correct implementation
    // for the current renderer API
    static WRef<IndexBuffer> create(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode = DrawMode::Static);

protected:
    W_ID unique_id_;
    uint32_t count_;
    DrawPrimitive primitive_;
};

// UBO
class UniformBuffer
{
public:
    UniformBuffer(const std::string& name, uint32_t struct_size): unique_id_(id::unique_id()), name_(name), struct_size_(struct_size) { }
    virtual ~UniformBuffer() = default;

    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual void map(void* data) = 0;

    inline const std::string& get_name() const { return name_; }
    inline uint32_t get_data_size() const      { return struct_size_; }
    // Return engine unique id for this object
    inline W_ID get_unique_id() const { return unique_id_; }

    // Factory method to create the correct implementation
    static WRef<UniformBuffer> create(const std::string& name, void* data, uint32_t struct_size, DrawMode mode = DrawMode::Dynamic);

protected:
    W_ID unique_id_;
    std::string name_;
    uint32_t struct_size_;
};

// SSBO / UAV
class ShaderStorageBuffer
{
public:
    ShaderStorageBuffer(const std::string& name, uint32_t count, uint32_t struct_size): unique_id_(id::unique_id()), name_(name), count_(count), struct_size_(struct_size) { }
    virtual ~ShaderStorageBuffer() = default;

    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual void stream(void* data, uint32_t count, std::size_t offset) = 0;
    virtual void map(void* data, uint32_t count) = 0;

    inline const std::string& get_name() const { return name_; }
    inline uint32_t get_count() const          { return count_; }
    inline uint32_t get_data_size() const      { return struct_size_; }
    // Return engine unique id for this object
    inline W_ID get_unique_id() const { return unique_id_; }

    // Factory method to create the correct implementation
    static WRef<ShaderStorageBuffer> create(const std::string& name, void* data, uint32_t count, uint32_t struct_size, DrawMode mode = DrawMode::Dynamic);

protected:
    W_ID unique_id_;
    std::string name_;
    uint32_t count_;
    uint32_t struct_size_;
};

class VertexArray
{
public:
    VertexArray(): unique_id_(id::unique_id()) {}
    virtual ~VertexArray() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    // Set the index buffer
    virtual void set_index_buffer(WRef<IndexBuffer> p_ib) = 0;
    // Add A vertex buffer, should use this for non-interleaved data
    virtual void add_vertex_buffer(WRef<VertexBuffer> p_vb) = 0;
    // Set THE vertex buffer, should use this for interleaved data
    inline void set_vertex_buffer(WRef<VertexBuffer> p_vb)
    {
        W_ASSERT(vertex_buffers_.size()==0, "[VertexArray] Use add_vertex_buffer() instead if you intend to add multiple vertex buffers (non-interleaved data).");
        add_vertex_buffer(p_vb);
    }

    inline const VertexBuffer& get_vertex_buffer(uint32_t index=0) const
    {
        W_ASSERT(index<vertex_buffers_.size(), "[VertexArray] Vertex buffer index out of bounds");
        return *vertex_buffers_[index];
    }
    inline VertexBuffer& get_vertex_buffer(uint32_t index=0)
    {
        W_ASSERT(index<vertex_buffers_.size(), "[VertexArray] Vertex buffer index out of bounds");
        return *vertex_buffers_[index];
    }
    inline const std::vector<WRef<VertexBuffer>>& get_vertex_buffers() const { return vertex_buffers_; }
    inline const IndexBuffer& get_index_buffer() const { return *index_buffer_; }
    inline IndexBuffer& get_index_buffer() { return *index_buffer_; }
    inline bool has_index_buffer() const { return (index_buffer_!=nullptr); }
    // Return engine unique id for this object
    inline W_ID get_unique_id() const { return unique_id_; }

    // Factory method to create the correct implementation
    // for the current renderer API
    static WRef<VertexArray> create();

protected:
    W_ID unique_id_;
	std::vector<WRef<VertexBuffer>> vertex_buffers_;
	WRef<IndexBuffer> index_buffer_ = nullptr;
};

} // namespace erwin