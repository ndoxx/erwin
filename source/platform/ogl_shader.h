#pragma once

#include <vector>

#include "core/core.h"
#include "render/shader.h"
#include "render/shader_lang.h"

#include "glm/glm.hpp"

namespace erwin
{

class OGLShader: public Shader
{
public:
	OGLShader() = default;
	~OGLShader() = default;

	// Initialize shader from packed GLSL source
	virtual bool init_glsl(const std::string& name, const fs::path& glsl_file) override;
	// Initialize shader from SPIR-V file
	virtual bool init_spirv(const std::string& name, const fs::path& spv_file) override;

	virtual void bind() const override;
	virtual void unbind() const override;
	virtual uint32_t get_texture_slot(hash_t sampler) const override;
	virtual uint32_t get_texture_count() const override;
	virtual void attach_texture_2D(const Texture2D& texture, int32_t slot) const override;

	virtual void attach_shader_storage(const ShaderStorageBuffer& buffer) override;
	virtual void attach_uniform_buffer(const UniformBuffer& buffer) override;

	virtual void bind_shader_storage(const ShaderStorageBuffer& buffer, uint32_t size=0, uint32_t base_offset=0) const override;
	virtual void bind_uniform_buffer(const UniformBuffer& buffer, uint32_t size=0, uint32_t offset=0) const override;

    virtual const BufferLayout& get_attribute_layout() const override;

    // Uniform management
    template <typename T>
    bool send_uniform(hash_t u_name, const T& value) const
    {
    	W_ASSERT(false, "Unknown uniform type!");
        return false;
    }

private:
	// Build the shader program from sources
	bool build(const std::vector<std::pair<slang::ExecutionModel, std::string>>& sources);
	// Build the shader program from SPIR-V binary
	bool build_spirv(const fs::path& filepath);
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

    uint32_t rd_handle_ = 0;
    uint32_t current_slot_ = 0;
    BufferLayout attribute_layout_;
    std::map<hash_t, int32_t> uniform_locations_; // [uniform hname, location]
    std::map<hash_t, uint32_t> texture_slots_;    // [uniform hname, slot]
    std::map<hash_t, uint32_t> block_bindings_;   // [block hname, binding point]
    std::map<W_ID, ResourceBinding> bound_buffers_;
    fs::path filepath_;
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