#include <string>
#include <cstring>

#include "platform/ogl_buffer.h"
#include "core/core.h"
#include "debug/logger.h"

#include "glad/glad.h"

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

static GLenum to_ogl_draw_mode(DrawMode mode)
{
	switch(mode)
    {
    	case DrawMode::Static:  return GL_STATIC_DRAW;
    	case DrawMode::Stream:  return GL_STREAM_DRAW;
    	case DrawMode::Dynamic: return GL_DYNAMIC_DRAW;
    }
}

OGLVertexBuffer::OGLVertexBuffer(float* vertex_data, uint32_t count, const BufferLayout& layout, DrawMode mode):
VertexBuffer(layout, count),
rd_handle_(0)
{
    GLenum gl_draw_mode = to_ogl_draw_mode(mode);

    glGenBuffers(1, &rd_handle_);
    glBindBuffer(GL_ARRAY_BUFFER, rd_handle_);
    glBufferData(GL_ARRAY_BUFFER, count_*sizeof(float), vertex_data, gl_draw_mode);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Vertex Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
    DLOGI << "Vertex count: " << count_ << std::endl;
    DLOGI << "Size:         " << count_*sizeof(float) << "B" << std::endl;
	DLOGI << "Layout:       ";
	for(auto&& element: layout)
	{
		DLOGI << "[" << to_string(element.type) << "]";
	}
	DLOGI << std::endl;
}

OGLVertexBuffer::~OGLVertexBuffer()
{
    // Unbind and delete
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &rd_handle_);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Vertex Buffer" << WCC(0) << " destroyed. id=" << rd_handle_ << std::endl;
}

void OGLVertexBuffer::bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, rd_handle_);
}

void OGLVertexBuffer::unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OGLVertexBuffer::stream(float* vertex_data, uint32_t count, std::size_t offset) const
{
    glBindBuffer(GL_ARRAY_BUFFER, rd_handle_);
    glBufferSubData(GL_ARRAY_BUFFER, (GLuint)offset, count*sizeof(float), vertex_data);
}

void OGLVertexBuffer::map(float* vertex_data, uint32_t count) const
{
    glBindBuffer(GL_ARRAY_BUFFER, rd_handle_);
	void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(ptr, vertex_data, count*sizeof(float));
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
    DLOGI << "Vertex count: " << count_ << std::endl;
    DLOGI << "Size:         " << count_*sizeof(float) << "B" << std::endl;
}

OGLIndexBuffer::~OGLIndexBuffer()
{
    // Unbind and delete
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &rd_handle_);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Index Buffer" << WCC(0) << " destroyed. id=" << rd_handle_ << std::endl;
}

void OGLIndexBuffer::bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd_handle_);
}

void OGLIndexBuffer::unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void OGLIndexBuffer::stream(uint32_t* index_data, uint32_t count, std::size_t offset) const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd_handle_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (GLuint)offset, count*sizeof(uint32_t), index_data);
}

void OGLIndexBuffer::map(uint32_t* index_data, uint32_t count) const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd_handle_);
	void* ptr = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(ptr, index_data, count*sizeof(uint32_t));
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}


// ----------------------------------------------------------------------------------

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
    glDeleteVertexArrays(1, &rd_handle_);
    DLOG("render",1) << "OpenGL " << WCC('i') << "Vertex Array" << WCC(0) << " destroyed. id=" << rd_handle_ << std::endl;
}

void OGLVertexArray::bind() const
{
    glBindVertexArray(rd_handle_);
}

void OGLVertexArray::unbind() const
{
    glBindVertexArray(0);
}

void OGLVertexArray::add_vertex_buffer(std::shared_ptr<VertexBuffer> p_vb)
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

	vertex_buffers_.push_back(p_vb);

	DLOG("render",1) << "Vertex array [" << rd_handle_ << "]: added vertex buffer ["
					 << std::static_pointer_cast<OGLVertexBuffer>(p_vb)->get_handle() << "]" << std::endl;
}

void OGLVertexArray::set_index_buffer(std::shared_ptr<IndexBuffer> p_ib)
{
    glBindVertexArray(rd_handle_);
	p_ib->bind();
	index_buffer_ = p_ib;

	DLOG("render",1) << "Vertex array [" << rd_handle_ << "]: set index buffer ["
					 << std::static_pointer_cast<OGLIndexBuffer>(p_ib)->get_handle() << "]" << std::endl;
}

} // namespace erwin