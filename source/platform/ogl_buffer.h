#pragma once

#include "render/buffer.h"

namespace erwin
{

class OGLVertexBuffer: public VertexBuffer
{
public:
    OGLVertexBuffer(float* vertex_data, std::size_t size, const BufferLayout& layout, bool dynamic=false);
    virtual ~OGLVertexBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;

    virtual void stream(float* vertex_data, std::size_t size, std::size_t offset) const override;

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

private:
    uint32_t rd_handle_;
    uint32_t vb_index_;
};

} // namespace erwin