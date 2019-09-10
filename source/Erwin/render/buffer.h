#pragma once

#include <memory>
#include <vector>

#include "core/wtypes.h"

namespace erwin
{

enum class ShaderDataType: uint8_t
{
    Float = 0, Vec2, Vec3, Vec4, Mat3, Mat4, Int, IVec2, IVec3, IVec4, Bool
};

struct BufferLayoutElement
{
    BufferLayoutElement() = default;
    BufferLayoutElement(hash_t name, ShaderDataType type, bool normalized=false);

    uint32_t get_component_count() const;

    hash_t name;
    ShaderDataType type;
    uint32_t size;
    uint32_t offset;
    bool normalized;
};

class BufferLayout
{
public:
    BufferLayout(const std::initializer_list<BufferLayoutElement>& elements);
    void compute_offset_and_stride();

    inline std::vector<BufferLayoutElement>::iterator begin()             { return elements_.begin(); }
    inline std::vector<BufferLayoutElement>::iterator end()               { return elements_.end(); }
    inline std::vector<BufferLayoutElement>::const_iterator begin() const { return elements_.begin(); }
    inline std::vector<BufferLayoutElement>::const_iterator end() const   { return elements_.end(); }

    inline uint32_t get_stride() const { return stride_; }
    inline size_t get_size() const     { return elements_.size(); }

private:
    std::vector<BufferLayoutElement> elements_;
    uint32_t stride_;
};

class VertexBuffer
{
public:
    VertexBuffer(const BufferLayout& layout): layout_(layout) {}
    virtual ~VertexBuffer() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void stream(float* vertex_data, std::size_t size, std::size_t offset) const = 0;

	inline const BufferLayout& get_layout() const { return layout_; }

    // Factory method to create the correct implementation
    // for the current renderer API
    static VertexBuffer* create(float* vertex_data, std::size_t size, const BufferLayout& layout, bool dynamic=false);

protected:
    BufferLayout layout_;
};

class IndexBuffer
{
public:
    IndexBuffer(uint32_t count): count_(count) {}
    virtual ~IndexBuffer() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void stream(uint32_t* index_data, uint32_t count, std::size_t offset) const = 0;

    inline uint32_t get_count() const { return count_; }

    static IndexBuffer* create(uint32_t* index_data, uint32_t count, bool dynamic=false);

protected:
    uint32_t count_;
};

//class BufferLayout;
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

    //virtual void set_layout(const BufferLayout& layout) const = 0;

    static VertexArray* create();

protected:
	std::vector<std::shared_ptr<VertexBuffer>> vertex_buffers_;
	std::shared_ptr<IndexBuffer> index_buffer_;
};

} // namespace erwin