#include <string>
#include <cstring>

#include "platform/ogl_buffer.h"
#include "core/core.h"
#include "debug/logger.h"

#include "glad/glad.h"

#include <iostream>

namespace erwin
{

static std::string to_string(ShaderDataType type)
{
	switch(type)
	{
		case ShaderDataType::Float: return "Float";
		case ShaderDataType::Vec2:  return "Vec2";
		case ShaderDataType::Vec3:  return "Vec3";
		case ShaderDataType::Vec4:  return "Vec4";
		case ShaderDataType::Mat3:  return "Mat3";
		case ShaderDataType::Mat4:  return "Mat4";
		case ShaderDataType::Int:   return "Int";
		case ShaderDataType::IVec2: return "IVec2";
		case ShaderDataType::IVec3: return "IVec3";
		case ShaderDataType::IVec4: return "IVec4";
		case ShaderDataType::Bool:  return "Bool";
	}
}

static std::string to_string(DrawMode mode)
{
	switch(mode)
    {
    	case DrawMode::Static:  return "Static";
    	case DrawMode::Stream:  return "Stream";
    	case DrawMode::Dynamic: return "Dynamic";
    }
}

static GLenum to_ogl_draw_mode(DrawMode mode)
{
	switch(mode)
    {
    	case DrawMode::Static:  return GL_STATIC_DRAW;
    	case DrawMode::Stream:  return GL_STREAM_DRAW;
    	case DrawMode::Dynamic: return GL_DYNAMIC_DRAW;
    }
}

#ifndef W_BUFFER_ALT

OGLVertexBuffer::OGLVertexBuffer(float* vertex_data, uint32_t count, const BufferLayout& layout, DrawMode mode):
VertexBuffer(layout, count),
rd_handle_(0)
{
    GLenum gl_draw_mode = to_ogl_draw_mode(mode);

    glGenBuffers(1, &rd_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, rd_handle_);
    glBufferData(GL_ARRAY_BUFFER, count_*layout_.get_stride(), vertex_data, gl_draw_mode);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Vertex Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
    DLOGI << "Vertex count: " << count_ << std::endl;
    DLOGI << "Size:         " << count_*layout_.get_stride() << "B" << std::endl;
    DLOGI << "Draw mode:    " << to_string(mode) << std::endl;
	DLOGI << "Layout:       ";
	for(auto&& element: layout_)
		DLOGI << "[" << to_string(element.type) << "]";
	DLOGI << std::endl;
}

OGLVertexBuffer::~OGLVertexBuffer()
{
    DLOG("render",1) << "Destroying OpenGL " << WCC('i') << "Vertex Buffer " << WCC(0) << " id=" << rd_handle_ << std::endl;
    // Unbind and delete
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &rd_handle_);
    DLOGI << "done" << std::endl;
}

void OGLVertexBuffer::bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, rd_handle_);
}

void OGLVertexBuffer::unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OGLVertexBuffer::stream(void* vertex_data, uint32_t size, uint32_t offset)
{
    glBindBuffer(GL_ARRAY_BUFFER, rd_handle_);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, vertex_data);
}

void OGLVertexBuffer::map(void* vertex_data, uint32_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, rd_handle_);
	void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(ptr, vertex_data, size);
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

// ----------------------------------------------------------------------------------

OGLIndexBuffer::OGLIndexBuffer(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode):
IndexBuffer(count, primitive),
rd_handle_(0)
{
    GLenum gl_draw_mode = to_ogl_draw_mode(mode);

    glGenBuffers(1, &rd_handle_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd_handle_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count_*sizeof(uint32_t), index_data, gl_draw_mode);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Index Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
    DLOGI << "Index count:  " << count_ << std::endl;
    DLOGI << "Size:         " << count_*sizeof(float) << "B" << std::endl;
    DLOGI << "Draw mode:    " << to_string(mode) << std::endl;
}

OGLIndexBuffer::~OGLIndexBuffer()
{
    DLOG("render",1) << "Destroying OpenGL " << WCC('i') << "Index Buffer" << WCC(0) << " id=" << rd_handle_ << std::endl;
    // Unbind and delete
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &rd_handle_);

    DLOGI << "done" << std::endl;
}

void OGLIndexBuffer::bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd_handle_);
}

void OGLIndexBuffer::unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void OGLIndexBuffer::stream(void* data, uint32_t size, uint32_t offset)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd_handle_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
}

void OGLIndexBuffer::map(void* data, uint32_t size)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd_handle_);
    void* ptr = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr, data, size);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}


// ----------------------------------------------------------------------------------

OGLUniformBuffer::OGLUniformBuffer(const std::string& name, void* data, uint32_t struct_size, DrawMode mode):
UniformBuffer(name, struct_size)
{
    GLenum gl_draw_mode = to_ogl_draw_mode(mode);

    glGenBuffers(1, &rd_handle_);
    glBindBuffer(GL_UNIFORM_BUFFER, rd_handle_);
    glBufferData(GL_UNIFORM_BUFFER, struct_size_, data, gl_draw_mode);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Uniform Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
    DLOGI << "Total size:    " << struct_size_ << "B" << std::endl;
}

OGLUniformBuffer::~OGLUniformBuffer()
{
    DLOG("render",1) << "Destroying OpenGL " << WCC('i') << "Uniform Buffer" << WCC(0) << " id=" << rd_handle_ << std::endl;
    // Unbind and delete
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glDeleteBuffers(1, &rd_handle_);

    DLOGI << "done" << std::endl;
}

void OGLUniformBuffer::bind() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, rd_handle_);
}

void OGLUniformBuffer::unbind() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OGLUniformBuffer::stream(void* data, uint32_t size, uint32_t offset)
{
    glBindBuffer(GL_UNIFORM_BUFFER, rd_handle_);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, (size!=0 ? size : struct_size_), data);
}

void OGLUniformBuffer::map(void* data, uint32_t size)
{
    glBindBuffer(GL_UNIFORM_BUFFER, rd_handle_);
    void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr, data, size);
    glUnmapBuffer(GL_UNIFORM_BUFFER);
}


// ----------------------------------------------------------------------------------

OGLShaderStorageBuffer::OGLShaderStorageBuffer(const std::string& name, void* data, uint32_t size, DrawMode mode):
ShaderStorageBuffer(name, size)
{
    GLenum gl_draw_mode = to_ogl_draw_mode(mode);

    glGenBuffers(1, &rd_handle_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rd_handle_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size_, data, gl_draw_mode);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Shader Storage Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
    DLOGI << "Size: " << size_ << "B" << std::endl;
}

OGLShaderStorageBuffer::~OGLShaderStorageBuffer()
{
    DLOG("render",1) << "Destroying OpenGL " << WCC('i') << "Shader Storage Buffer" << WCC(0) << " id=" << rd_handle_ << std::endl;
    // Unbind and delete
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glDeleteBuffers(1, &rd_handle_);

    DLOGI << "done" << std::endl;
}

void OGLShaderStorageBuffer::bind() const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rd_handle_);
}

void OGLShaderStorageBuffer::unbind() const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void OGLShaderStorageBuffer::stream(void* data, uint32_t size, uint32_t offset)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rd_handle_);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
}

void OGLShaderStorageBuffer::map(void* data, uint32_t size)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rd_handle_);
	void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	memcpy(ptr, data, size_);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

// ----------------------------------------------------------------------------------

#else



#endif


static GLenum to_ogl_base_type(ShaderDataType type)
{
	switch(type)
	{
		case erwin::ShaderDataType::Float: return GL_FLOAT;
		case erwin::ShaderDataType::Vec2:  return GL_FLOAT;
		case erwin::ShaderDataType::Vec3:  return GL_FLOAT;
		case erwin::ShaderDataType::Vec4:  return GL_FLOAT;
		case erwin::ShaderDataType::Mat3:  return GL_FLOAT;
		case erwin::ShaderDataType::Mat4:  return GL_FLOAT;
		case erwin::ShaderDataType::Int:   return GL_INT;
		case erwin::ShaderDataType::IVec2: return GL_INT;
		case erwin::ShaderDataType::IVec3: return GL_INT;
		case erwin::ShaderDataType::IVec4: return GL_INT;
		case erwin::ShaderDataType::Bool:  return GL_BOOL;
	}

	DLOGE("render") << "Unknown ShaderDataType!" << std::endl;
	return 0;
}

OGLVertexArray::OGLVertexArray()
{
    //glGenVertexArrays(1, &rd_handle_);
    glCreateVertexArrays(1, &rd_handle_);
    DLOG("render",1) << "OpenGL " << WCC('i') << "Vertex Array" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
}

OGLVertexArray::~OGLVertexArray()
{
    DLOG("render",1) << "Destroying OpenGL " << WCC('i') << "Vertex Array" << WCC(0) << " id=" << rd_handle_ << std::endl;
    glDeleteVertexArrays(1, &rd_handle_);
    DLOGI << "done" << std::endl;
}

void OGLVertexArray::bind() const
{
    glBindVertexArray(rd_handle_);
}

void OGLVertexArray::unbind() const
{
    glBindVertexArray(0);
}

void OGLVertexArray::add_vertex_buffer(WRef<VertexBuffer> p_vb)
{
	// Make sure buffer layout is meaningful
	W_ASSERT(p_vb->get_layout().get_count(), "Vertex buffer has empty layout!");

	// Bind vertex array then vertex buffer
    glBindVertexArray(rd_handle_);
	p_vb->bind();

	// For each element in layout, enable attribute array and push 
	// data layout description to OpenGL
	const auto& layout = p_vb->get_layout();
	for(auto&& element: layout)
	{
		glEnableVertexAttribArray(vb_index_);
		glVertexAttribPointer(vb_index_,
							  element.get_component_count(),
							  to_ogl_base_type(element.type),
							  element.normalized ? GL_TRUE : GL_FALSE,
							  layout.get_stride(),
							  (const void*)(intptr_t)element.offset);
		++vb_index_;
	}
    glBindVertexArray(0); // Very important, state leak here can lead to segfault during draw calls

	vertex_buffers_.push_back(p_vb);

	DLOG("render",1) << "Vertex array [" << rd_handle_ << "]: added vertex buffer ["
					 << std::static_pointer_cast<OGLVertexBuffer>(p_vb)->get_handle() << "]" << std::endl;
}

void OGLVertexArray::set_index_buffer(WRef<IndexBuffer> p_ib)
{
    glBindVertexArray(rd_handle_);
	p_ib->bind();
    glBindVertexArray(0); // Very important, state leak here can lead to segfault during draw calls
	
    index_buffer_ = p_ib;

	DLOG("render",1) << "Vertex array [" << rd_handle_ << "]: set index buffer ["
					 << std::static_pointer_cast<OGLIndexBuffer>(p_ib)->get_handle() << "]" << std::endl;
}

// ----------------------------------------------------------------------------------




} // namespace erwin