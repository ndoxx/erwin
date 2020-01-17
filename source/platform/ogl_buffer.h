#pragma once

#include "render/buffer.h"

namespace erwin
{

#ifndef W_BUFFER_ALT

class OGLVertexBuffer: public VertexBuffer
{
public:
    OGLVertexBuffer(float* vertex_data, uint32_t count, const BufferLayout& layout, DrawMode mode);
    virtual ~OGLVertexBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;
    virtual void stream(void* data, uint32_t size, uint32_t offset) override;
    virtual void map(void* data, uint32_t size) override;

    inline uint32_t get_handle() const { return rd_handle_; }

private:
    uint32_t rd_handle_;
};

class OGLIndexBuffer: public IndexBuffer
{
public:
    OGLIndexBuffer(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode);
    virtual ~OGLIndexBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;
    virtual void stream(void* data, uint32_t size, uint32_t offset) override;
    virtual void map(void* data, uint32_t size) override;

    inline uint32_t get_handle() const { return rd_handle_; }

private:
    uint32_t rd_handle_;
};

class OGLUniformBuffer: public UniformBuffer
{
public:
    OGLUniformBuffer(const std::string& name, void* data, uint32_t struct_size, DrawMode mode);
    virtual ~OGLUniformBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;
    virtual void stream(void* data, uint32_t size, uint32_t offset) override;
    virtual void map(void* data, uint32_t size) override;

    inline uint32_t get_handle() const { return rd_handle_; }

protected:
    uint32_t rd_handle_;
};

class OGLShaderStorageBuffer: public ShaderStorageBuffer
{
public:
    OGLShaderStorageBuffer(const std::string& name, void* data, uint32_t size, DrawMode mode);
    virtual ~OGLShaderStorageBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;
    virtual void stream(void* data, uint32_t size, uint32_t offset) override;
    virtual void map(void* data, uint32_t size) override;

    inline uint32_t get_handle() const { return rd_handle_; }

protected:
    uint32_t rd_handle_;
};





#else



#endif

class OGLVertexArray: public VertexArray
{
public:
    OGLVertexArray();
    virtual ~OGLVertexArray();

    virtual void bind() const override;
    virtual void unbind() const override;

    virtual void add_vertex_buffer(WRef<VertexBuffer> p_vb) override;
    virtual void set_index_buffer(WRef<IndexBuffer> p_ib) override;
    
    inline uint32_t get_handle() const { return rd_handle_; }

private:
    uint32_t rd_handle_;
    uint32_t vb_index_ = 0;
};

} // namespace erwin