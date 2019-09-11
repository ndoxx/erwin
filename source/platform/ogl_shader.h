#pragma once

#include <vector>

#include "render/shader.h"

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

private:
	// Helper function to extract the various shader sources from a single formatted source string
	std::vector<std::pair<ShaderType, std::string>> parse(const std::string& full_source);
	// Build the shader program from sources
	bool build(const std::vector<std::pair<ShaderType, std::string>>& sources);
	// Helper function to construct the uniform registry
	void setup_uniform_registry();

private:
    uint32_t rd_handle_;
    std::map<hash_t, int32_t> uniform_locations_; // [uniform hname, location]
};

} // namespace erwin