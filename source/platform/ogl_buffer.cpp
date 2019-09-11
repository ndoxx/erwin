#include "platform/ogl_buffer.h"
#include "core/core.h"
#include "debug/logger.h"

#include "glad/glad.h"

namespace erwin
{

OGLVertexBuffer::OGLVertexBuffer(float* vertex_data, uint32_t count, const BufferLayout& layout, bool dynamic):
VertexBuffer(layout, count),
rd_handle_(0)
{
    GLenum draw_type = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    glGenBuffers(1, &rd_handle_);
    bind();
    glBufferData(GL_ARRAY_BUFFER, count*sizeof(float), vertex_data, draw_type);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Vertex Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
}

OGLVertexBuffer::~OGLVertexBuffer()
{
    // Unbind and delete
    unbind();
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
    bind();
    glBufferSubData(GL_ARRAY_BUFFER, (GLuint)offset, count*sizeof(float), vertex_data);
}

// ----------------------------------------------------------------------------------

OGLIndexBuffer::OGLIndexBuffer(uint32_t* index_data, uint32_t count, bool dynamic):
IndexBuffer(count),
rd_handle_(0)
{
    GLenum draw_type = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    glGenBuffers(1, &rd_handle_);
    bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count_*sizeof(uint32_t), index_data, draw_type);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Index Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
}

OGLIndexBuffer::~OGLIndexBuffer()
{
    // Unbind and delete
    unbind();
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
    bind();
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (GLuint)offset, count*sizeof(uint32_t), index_data);
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
    glGenVertexArrays(1, &rd_handle_);
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
	if(!p_vb->get_layout().get_count())
	{
		DLOGF("render") << "Vertex buffer has empty layout!" << std::endl;
		fatal();
	}

	// Bind vertex array then vertex buffer
	bind();
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
}

void OGLVertexArray::set_index_buffer(std::shared_ptr<IndexBuffer> p_ib)
{
	bind();
	p_ib->bind();
	index_buffer_ = p_ib;
}

} // namespace erwin