#pragma once

#include "render/buffer_layout.h"
#include "core/unique_id.h"

namespace erwin
{

class OGLBuffer
{
public:
    virtual ~OGLBuffer();

    void init_base(uint32_t target, void* data, uint32_t size, UsagePattern mode);
    void release_base();
    void bind() const;
    void unbind() const;
    void stream(void* data, uint32_t size, uint32_t offset=0);
    void map(void* data, uint32_t size, uint32_t offset=0);
    void map_persistent(void* data, uint32_t size, uint32_t offset=0);

    inline uint32_t get_handle() const          { return rd_handle_; }
    inline W_ID get_unique_id() const           { return unique_id_; }
    inline bool has_persistent_mapping() const  { return has_persistent_mapping_; }
    inline void* get_persistent_pointer() const { return persistent_map_; }

protected:
    W_ID unique_id_ = 0;
    void* persistent_map_ = nullptr;
    bool has_persistent_mapping_ = false;
    bool initialized_ = false;
    uint32_t target_ = 0;
    uint32_t rd_handle_ = 0;
};

class OGLVertexBuffer: public OGLBuffer
{
public:
    OGLVertexBuffer() = default;
    OGLVertexBuffer(float* vertex_data, uint32_t count, const BufferLayout& layout, UsagePattern mode);
    virtual ~OGLVertexBuffer() = default;

    void init(float* vertex_data, uint32_t count, const BufferLayout& layout, UsagePattern mode);
    void release();

    // Return the buffer layout
    inline const BufferLayout& get_layout() const { return layout_; }
    // Return the vertex count of this vertex buffer
    inline std::size_t get_count() const          { return count_; }
    // Return the size (in bytes) of this vertex buffer
    inline std::size_t get_size() const           { return count_*sizeof(float); }

private:
    BufferLayout layout_ = {};
    std::size_t count_ = 0;
};

class OGLIndexBuffer: public OGLBuffer
{
public:
    OGLIndexBuffer() = default;
    OGLIndexBuffer(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, UsagePattern mode);
    virtual ~OGLIndexBuffer() = default;

    void init(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, UsagePattern mode);
    void release();

    // Return the number of indices
    inline uint32_t get_count() const          { return count_; }
    // Return the size (in bytes) of this buffer
    inline uint32_t get_size() const           { return count_*sizeof(uint32_t); }
    // Return the intended draw primitive
    inline DrawPrimitive get_primitive() const { return primitive_; }

private:
    DrawPrimitive primitive_ = DrawPrimitive::Triangles;
    uint32_t count_ = 0;
};

class OGLUniformBuffer: public OGLBuffer
{
public:
    OGLUniformBuffer() = default;
    OGLUniformBuffer(const std::string& name, void* data, uint32_t struct_size, UsagePattern mode);
    virtual ~OGLUniformBuffer() = default;

    void init(const std::string& name, void* data, uint32_t struct_size, UsagePattern mode);
    void release();

    inline const std::string& get_name() const { return name_; }
    inline uint32_t get_size() const           { return struct_size_; }

private:
    std::string name_;
    uint32_t struct_size_ = 0;
};

class OGLShaderStorageBuffer: public OGLBuffer
{
public:
    OGLShaderStorageBuffer() = default;
    OGLShaderStorageBuffer(const std::string& name, void* data, uint32_t size, UsagePattern mode);
    virtual ~OGLShaderStorageBuffer() = default;

    void init(const std::string& name, void* data, uint32_t size, UsagePattern mode);
    void release();

    inline const std::string& get_name() const { return name_; }
    inline uint32_t get_size() const           { return size_; }

private:
    std::string name_;
    uint32_t size_ = 0;
};


class OGLVertexArray
{
public:
    OGLVertexArray() = default;
    ~OGLVertexArray();

    void init();
    void release();

    void bind() const;
    void unbind() const;

    // Set the index buffer
    void set_index_buffer(std::reference_wrapper<OGLIndexBuffer> r_ib);
    // Add A vertex buffer, should use this for non-interleaved data
    void add_vertex_buffer(std::reference_wrapper<OGLVertexBuffer> r_vb);
    // Set THE vertex buffer, should use this for interleaved data
    inline void set_vertex_buffer(std::reference_wrapper<OGLVertexBuffer> r_vb)
    {
        K_ASSERT(vertex_buffers_.size()==0, "[OGLVertexArray] Use add_vertex_buffer() instead if you intend to add multiple vertex buffers (non-interleaved data).");
        add_vertex_buffer(r_vb);
    }

    inline const OGLVertexBuffer& get_vertex_buffer(uint32_t index=0) const
    {
        K_ASSERT(index<vertex_buffers_.size(), "[OGLVertexArray] Vertex buffer index out of bounds");
        return vertex_buffers_[index];
    }
    inline OGLVertexBuffer& get_vertex_buffer(uint32_t index=0)
    {
        K_ASSERT(index<vertex_buffers_.size(), "[OGLVertexArray] Vertex buffer index out of bounds");
        return vertex_buffers_[index];
    }

    inline const auto& get_vertex_buffers() const { return vertex_buffers_; }
    inline const OGLIndexBuffer& get_index_buffer() const { return *index_buffer_; }
    inline OGLIndexBuffer& get_index_buffer() { return *index_buffer_; }
    inline bool has_index_buffer() const { return index_buffer_.has_value(); }
    // Return engine unique id for this object
    inline W_ID get_unique_id() const { return unique_id_; }
    inline uint32_t get_handle() const { return rd_handle_; }

protected:
    std::vector<std::reference_wrapper<OGLVertexBuffer>> vertex_buffers_;
    std::optional<std::reference_wrapper<OGLIndexBuffer>> index_buffer_;
    uint32_t rd_handle_ = 0;
    uint32_t vb_index_ = 0;
    W_ID unique_id_ = 0;
    bool initialized_ = false;
};

} // namespace erwin