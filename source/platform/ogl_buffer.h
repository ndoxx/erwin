#pragma once

#include "render/buffer.h"

namespace erwin
{

class OGLVertexBuffer: public VertexBuffer
{
public:
    OGLVertexBuffer(float* vertex_data, uint32_t count, const BufferLayout& layout, bool dynamic=false);
    virtual ~OGLVertexBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;

    virtual void stream(float* vertex_data, uint32_t count, std::size_t offset) const override;

    inline uint32_t get_handle() const { return rd_handle_; }

private:
    uint32_t rd_handle_;
};

class OGLIndexBuffer: public IndexBuffer
{
public:
    OGLIndexBuffer(uint32_t* index_data, uint32_t count, bool dynamic=false);
    virtual ~OGLIndexBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;

    virtual void stream(uint32_t* index_data, uint32_t count, std::size_t offset) const override;
    
    inline uint32_t get_handle() const { return rd_handle_; }

private:
    uint32_t rd_handle_;
};

class OGLVertexArray: public VertexArray
{
public:
    OGLVertexArray();
    virtual ~OGLVertexArray();

    virtual void bind() const override;
    virtual void unbind() const override;

    virtual void add_vertex_buffer(std::shared_ptr<VertexBuffer> p_vb) override;
    virtual void set_index_buffer(std::shared_ptr<IndexBuffer> p_ib) override;
    
    inline uint32_t get_handle() const { return rd_handle_; }

private:
    uint32_t rd_handle_;
    uint32_t vb_index_ = 0;
};

} // namespace erwin