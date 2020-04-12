#pragma once

#include <memory>
#include <vector>

#include "core/core.h"
#include "core/unique_id.h"
#include "render/buffer_layout.h"
#include "memory/arena.h"

namespace erwin
{

/*
    Base class for all graphics buffers, allows for a single per-API implementation
    of buffer access methods. API-dependent buffer implementations use multiple
    inheritance, that's why all specializations below inherit virtually from GfxBuffer.
    In theory, this scheme should be compatible with DX and Vulkan.
*/
class GfxBuffer
{
public:
    GfxBuffer():
    unique_id_(id::unique_id()),
    persistent_map_(nullptr),
    has_persistent_mapping_(false)
    {}
    virtual ~GfxBuffer() = default;

    virtual void init(void* data, uint32_t size, UsagePattern mode) = 0;
    virtual void bind() const = 0;
    virtual void unbind() const = 0;
    virtual void stream(void* data, uint32_t size, uint32_t offset=0) = 0;
    virtual void map(void* data, uint32_t size, uint32_t offset=0) = 0;
    virtual void map_persistent(void* data, uint32_t size, uint32_t offset=0) = 0;

    inline W_ID get_unique_id() const           { return unique_id_; }
    inline bool has_persistent_mapping() const  { return has_persistent_mapping_; }
    inline void* get_persistent_pointer() const { return persistent_map_; }

protected:
    W_ID unique_id_;
    void* persistent_map_;
    bool has_persistent_mapping_;
};

class VertexBuffer: public virtual GfxBuffer
{
public:
    VertexBuffer(const BufferLayout& layout, uint32_t count): layout_(layout), count_(count) {}
    virtual ~VertexBuffer() = default;

    // Return the buffer layout
    inline const BufferLayout& get_layout() const { return layout_; }
    // Return the vertex count of this vertex buffer
    inline std::size_t get_count() const          { return count_; }
    // Return the size (in bytes) of this vertex buffer
    inline std::size_t get_size() const           { return count_*sizeof(float); }

    // Factory method to create the correct implementation
    // for the current renderer API
    static WRef<VertexBuffer> create(float* vertex_data, uint32_t count, const BufferLayout& layout, UsagePattern mode = UsagePattern::Static);
    static VertexBuffer* create(PoolArena& pool, float* vertex_data, uint32_t count, const BufferLayout& layout, UsagePattern mode = UsagePattern::Static);
    static size_t node_size();
protected:
    BufferLayout layout_;
    std::size_t count_;
};

class IndexBuffer: public virtual GfxBuffer
{
public:
    IndexBuffer(uint32_t count, DrawPrimitive primitive): primitive_(primitive), count_(count) {}
    virtual ~IndexBuffer() = default;

    // Return the number of indices
    inline uint32_t get_count() const          { return count_; }
    // Return the size (in bytes) of this buffer
    inline uint32_t get_size() const           { return count_*sizeof(uint32_t); }
    // Return the intended draw primitive
    inline DrawPrimitive get_primitive() const { return primitive_; }

    // Factory method to create the correct implementation
    // for the current renderer API
    static WRef<IndexBuffer> create(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, UsagePattern mode = UsagePattern::Static);
    static IndexBuffer* create(PoolArena& pool, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, UsagePattern mode = UsagePattern::Static);
    static size_t node_size();

protected:
    DrawPrimitive primitive_;
    uint32_t count_;
};

class UniformBuffer: public virtual GfxBuffer
{
public:
    UniformBuffer(const std::string& name, uint32_t struct_size): name_(name), struct_size_(struct_size) { }
    virtual ~UniformBuffer() = default;

    inline const std::string& get_name() const { return name_; }
    inline uint32_t get_size() const           { return struct_size_; }

    // Factory method to create the correct implementation
    static WRef<UniformBuffer> create(const std::string& name, void* data, uint32_t struct_size, UsagePattern mode = UsagePattern::Dynamic);
    static UniformBuffer* create(PoolArena& pool, const std::string& name, void* data, uint32_t struct_size, UsagePattern mode = UsagePattern::Dynamic);
    static size_t node_size();

protected:
    std::string name_;
    uint32_t struct_size_;
};

class ShaderStorageBuffer: public virtual GfxBuffer
{
public:
    ShaderStorageBuffer(const std::string& name, uint32_t size): name_(name), size_(size) { }
    virtual ~ShaderStorageBuffer() = default;

    inline const std::string& get_name() const { return name_; }
    inline uint32_t get_size() const           { return size_; }

    // Factory method to create the correct implementation
    static WRef<ShaderStorageBuffer> create(const std::string& name, void* data, uint32_t size, UsagePattern mode = UsagePattern::Dynamic);
    static ShaderStorageBuffer* create(PoolArena& pool, const std::string& name, void* data, uint32_t size, UsagePattern mode = UsagePattern::Dynamic);
    static size_t node_size();

protected:
    std::string name_;
    uint32_t size_;
};

class VertexArray
{
public:
    VertexArray(): unique_id_(id::unique_id()) {}
    virtual ~VertexArray() = default;

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
    static VertexArray* create(PoolArena& pool);
    static size_t node_size();

protected:
    W_ID unique_id_;
    std::vector<WRef<VertexBuffer>> vertex_buffers_;
    WRef<IndexBuffer> index_buffer_ = nullptr;
};
} // namespace erwin