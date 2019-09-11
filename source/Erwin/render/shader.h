#pragma once

#include <istream>
#include <string>
#include <memory>

namespace erwin
{

enum class ShaderType: uint8_t
{
	None = 0,
	Vertex,
	Geometry,
	Fragment,
};

class Shader
{
public:
	Shader(const std::string& name): name_(name) { }
	virtual ~Shader() = default;

	// Use this program
	virtual void bind() const = 0;
	// Stop using this program
	virtual void unbind() const = 0;

	// Return program debug name
	inline const std::string& get_name() const { return name_; }

	// Factory method for the creation of an API-specific shader from an input stream
	static std::shared_ptr<Shader> create(const std::string& name, std::istream& source_stream);
	// Factory method for the creation of an API-specific shader from a string
	static std::shared_ptr<Shader> create(const std::string& name, const std::string& source_string);

protected:
	std::string name_;
};


} // namespace erwin