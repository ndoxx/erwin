#pragma once

#include <istream>
#include <string>
#include <memory>
#include <unordered_map>

#include "core/wtypes.h"
#include "core/file_system.h"

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

// The shader bank holds references to multiple shaders and makes them accessible by name
class ShaderBank
{
public:
	// Add an existing shader to the bank
	void add(std::shared_ptr<Shader> p_shader);
	// Load shader in bank from path (relative to the asset directory)
	void load(const fs::path& path);
	// Load shader in bank from stream
	void load(const std::string& name, std::istream& source_stream);
	// Load shader in bank from string
	void load(const std::string& name, const std::string& source_string);
	// Get shader by hash name
	const Shader& get(hash_t name) const;
	// Check whether a shader is registered to this name
	bool exists(hash_t name) const;

private:
	std::unordered_map<hash_t, std::shared_ptr<Shader>> bank_;
};

// TMP: shader bank instance will be owned by renderer
static ShaderBank SHADER_BANK;

} // namespace erwin