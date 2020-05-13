#include <string>
#include <cstring>

#include "platform/ogl_buffer.h"
#include "core/core.h"
#include "debug/logger.h"

#include "glad/glad.h"

#include <iostream>

namespace erwin
{

[[maybe_unused]] static std::string to_string(ShaderDataType type)
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

[[maybe_unused]] static std::string to_string(UsagePattern mode)
{
	switch(mode)
    {
    	case UsagePattern::Static:            return "Static";
    	case UsagePattern::Stream:            return "Stream";
    	case UsagePattern::Dynamic:           return "Dynamic";
        case UsagePattern::PersistentMapping: return "PersistentMapping";
    }
}

static GLenum to_ogl_usage_pattern(UsagePattern mode)
{
	switch(mode)
    {
    	case UsagePattern::Static:            return GL_STATIC_DRAW;
    	case UsagePattern::Stream:            return GL_STREAM_DRAW;
    	case UsagePattern::Dynamic:           return GL_DYNAMIC_DRAW;
        case UsagePattern::PersistentMapping: return GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    }
}

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


OGLBuffer::OGLBuffer(uint32_t target):
unique_id_(id::unique_id()),
persistent_map_(nullptr),
has_persistent_mapping_(false),
target_(target),
rd_handle_(0)
{

}

OGLBuffer::~OGLBuffer()
{
    DLOG("render",1) << "Destroying OpenGL " << WCC('i') << "Buffer " << WCC(0) << " id=" << rd_handle_ << std::endl;
    // Unbind and delete
    glBindBuffer(target_, 0);
    glDeleteBuffers(1, &rd_handle_);
    DLOGI << "done" << std::endl;
}

void OGLBuffer::bind() const
{
    glBindBuffer(target_, rd_handle_);
}

void OGLBuffer::unbind() const
{
    glBindBuffer(target_, 0);
}

void OGLBuffer::stream(void* data, uint32_t size, uint32_t offset)
{
    glBindBuffer(target_, rd_handle_);
    glBufferSubData(target_, offset, size, data);
}

void OGLBuffer::map(void* data, uint32_t size, uint32_t offset)
{
    glBindBuffer(target_, rd_handle_);
    uint8_t* ptr = static_cast<uint8_t*>(glMapBuffer(target_, GL_WRITE_ONLY));
    memcpy(ptr+offset, data, size);
    glUnmapBuffer(target_);
}

void OGLBuffer::map_persistent(void* data, uint32_t size, uint32_t offset)
{
    W_ASSERT(has_persistent_mapping_, "Persistent mapping not enabled.");
    
    // Wait for the buffer
    GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000);

    // Update data
    memcpy(static_cast<uint8_t*>(persistent_map_)+offset, data, size);
    glDeleteSync(fence);
}

void OGLBuffer::init(void* data, uint32_t size, UsagePattern mode)
{
    GLenum gl_usage_pattern = to_ogl_usage_pattern(mode);

    glGenBuffers(1, &rd_handle_);
    glBindBuffer(target_, rd_handle_);
    if(mode != UsagePattern::PersistentMapping)
        glBufferData(target_, size, data, gl_usage_pattern);
    else
    {
        glBufferStorage(target_, size, 0, gl_usage_pattern | GL_DYNAMIC_STORAGE_BIT);
        persistent_map_ = glMapBufferRange(target_, 0, size, gl_usage_pattern);
        has_persistent_mapping_ = true;
    }
}


OGLVertexBuffer::OGLVertexBuffer(float* vertex_data, uint32_t count, const BufferLayout& layout, UsagePattern mode):
OGLBuffer(GL_ARRAY_BUFFER),
layout_(layout),
count_(count)
{
    uint32_t size = uint32_t(count_)*layout_.get_stride();
    init(vertex_data, size, mode);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Vertex Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
    DLOGI << "Vertex count:  " << count_ << std::endl;
    DLOGI << "Size:          " << size << "B" << std::endl;
    DLOGI << "Usage pattern: " << to_string(mode) << std::endl;
    DLOGI << "Layout:        ";
    for(auto&& element: layout_)
        DLOGI << "[" << to_string(element.type) << "]";
    DLOGI << std::endl;
}

OGLIndexBuffer::OGLIndexBuffer(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, UsagePattern mode):
OGLBuffer(GL_ELEMENT_ARRAY_BUFFER),
primitive_(primitive),
count_(count)
{
    uint32_t size = uint32_t(count_)*sizeof(uint32_t);
    init(index_data, size, mode);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Index Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
    DLOGI << "Index count:   " << count_ << std::endl;
    DLOGI << "Size:          " << count_*sizeof(float) << "B" << std::endl;
    DLOGI << "Usage pattern: " << to_string(mode) << std::endl;
}

OGLUniformBuffer::OGLUniformBuffer(const std::string& name, void* data, uint32_t struct_size, UsagePattern mode):
OGLBuffer(GL_UNIFORM_BUFFER),
name_(name),
struct_size_(struct_size)
{
    init(data, struct_size_, mode);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Uniform Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
    DLOGI << "Total size:    " << struct_size_ << "B" << std::endl;
}

OGLShaderStorageBuffer::OGLShaderStorageBuffer(const std::string& name, void* data, uint32_t size, UsagePattern mode):
OGLBuffer(GL_SHADER_STORAGE_BUFFER),
name_(name),
size_(size)
{
    init(data, size_, mode);

    DLOG("render",1) << "OpenGL " << WCC('i') << "Shader Storage Buffer" << WCC(0) << " created. id=" << rd_handle_ << std::endl;
    DLOGI << "Size: " << size_ << "B" << std::endl;
}

OGLVertexArray::OGLVertexArray():
unique_id_(id::unique_id())
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

void OGLVertexArray::add_vertex_buffer(WRef<OGLVertexBuffer> p_vb)
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
							  reinterpret_cast<const void*>(intptr_t(element.offset)));
		++vb_index_;
	}
    glBindVertexArray(0); // Very important, state leak here can lead to segfault during draw calls

	vertex_buffers_.push_back(p_vb);

	DLOG("render",1) << "Vertex array [" << rd_handle_ << "]: added vertex buffer ["
					 << std::static_pointer_cast<OGLVertexBuffer>(p_vb)->get_handle() << "]" << std::endl;
}

void OGLVertexArray::set_index_buffer(WRef<OGLIndexBuffer> p_ib)
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