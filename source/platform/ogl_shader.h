#pragma once

#include <vector>

#include "core/core.h"
#include "render/shader.h"

#include "glm/glm.hpp"

namespace erwin
{

class OGLShader: public Shader
{
public:
	OGLShader(const std::string& name, std::istream& source_stream);
	OGLShader(const std::string& name, const std::string& source_string);
	~OGLShader();

	virtual void bind() const override;
	virtual void unbind() const override;
	virtual uint32_t get_texture_slot(hash_t sampler) const override;
	virtual void attach_shader_storage(const ShaderStorageBuffer& buffer, const std::string& name) const override;

    // Uniform management
    template <typename T>
    bool send_uniform(hash_t u_name, const T& value) const
    {
    	W_ASSERT(false, "Unknown uniform type!");
        return false;
    }

private:
	// Helper function to extract the various shader sources from a single formatted source string
	std::vector<std::pair<ShaderType, std::string>> parse(const std::string& full_source);
	// Build the shader program from sources
	bool build(const std::vector<std::pair<ShaderType, std::string>>& sources);
	// Helper function to construct the uniform location and texture slot registries
	void setup_uniform_registry();

private:
    uint32_t rd_handle_ = 0;
    uint32_t current_slot_ = 0;
    std::unordered_map<hash_t, int32_t> uniform_locations_; // [uniform hname, location]
    std::unordered_map<hash_t, uint32_t> texture_slots_;    // [uniform hname, slot]
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