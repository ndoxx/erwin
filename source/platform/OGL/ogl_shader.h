#pragma once

#include <vector>
#include <map>

#include "core/core.h"
#include "render/shader_lang.h"
#include "platform/OGL/ogl_buffer.h"
#include "platform/OGL/ogl_texture.h"
#include "filesystem/wpath.h"

#include "glm/glm.hpp"

namespace erwin
{

class OGLShader
{
public:
	OGLShader() = default;
	~OGLShader() = default;

	bool init(const std::string& name, const WPath& filepath);
	// Initialize shader from packed GLSL source
	bool init_glsl(const std::string& name, const WPath& glsl_file);
	// Initialize shader from SPIR-V file
	bool init_spirv(const std::string& name, const WPath& spv_file);

	void bind() const;
	void unbind() const;
	uint32_t get_texture_slot(hash_t sampler) const;
	uint32_t get_texture_count() const;
	void attach_texture_2D(const OGLTexture2D& texture, uint32_t slot) const;
	void attach_cubemap(const OGLCubemap& cubemap, uint32_t slot) const;

	void attach_shader_storage(const OGLShaderStorageBuffer& buffer);
	void attach_uniform_buffer(const OGLUniformBuffer& buffer);

	void bind_shader_storage(const OGLShaderStorageBuffer& buffer, uint32_t size=0, uint32_t base_offset=0) const;
	void bind_uniform_buffer(const OGLUniformBuffer& buffer, uint32_t size=0, uint32_t offset=0) const;

    const BufferLayout& get_attribute_layout() const;

	// Return program debug name
	inline const std::string& get_name() const { return name_; }

    // Uniform management
    template <typename T>
    bool send_uniform(hash_t, const T&) const
    {
    	K_ASSERT(false, "Unknown uniform type!");
        return false;
    }

private:
	// Build the shader program from sources
	bool build(const std::vector<std::pair<slang::ExecutionModel, std::string>>& sources);
	// Build the shader program from SPIR-V binary
	bool build_spirv(const WPath& filepath);
	// Link the program
	bool link(const std::vector<uint32_t>& shader_ids);
	// Helper function to construct the uniform location, texture slot and binding point registries
	void introspect();

private:
	struct ResourceBinding
	{
		int32_t binding_point;
		uint32_t render_handle;
		uint32_t target;
	};
	std::string name_;

    uint32_t rd_handle_ = 0;
    uint32_t current_slot_ = 0;
    BufferLayout attribute_layout_;
    std::map<hash_t, int32_t> uniform_locations_; // [uniform hname, location]
    std::map<hash_t, uint32_t> texture_slots_;    // [uniform hname, slot]
    std::map<hash_t, uint32_t> block_bindings_;   // [block hname, binding point]
    std::map<W_ID, ResourceBinding> bound_buffers_;
    WPath filepath_;
};

template <> bool OGLShader::send_uniform<bool>(hash_t name, const bool& value) const;
template <> bool OGLShader::send_uniform<float>(hash_t name, const float& value) const;
template <> bool OGLShader::send_uniform<int>(hash_t name, const int& value) const;
template <> bool OGLShader::send_uniform<glm::vec2>(hash_t name, const glm::vec2& value) const;
template <> bool OGLShader::send_uniform<glm::vec3>(hash_t name, const glm::vec3& value) const;
template <> bool OGLShader::send_uniform<glm::vec4>(hash_t name, const glm::vec4& value) const;
template <> bool OGLShader::send_uniform<glm::mat2>(hash_t name, const glm::mat2& value) const;
template <> bool OGLShader::send_uniform<glm::mat3>(hash_t name, const glm::mat3& value) const;
template <> bool OGLShader::send_uniform<glm::mat4>(hash_t name, const glm::mat4& value) const;

} // namespace erwin