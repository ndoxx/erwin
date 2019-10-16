#pragma once

#include <istream>
#include <string>
#include <memory>
#include <map>

#include "core/wtypes.h"
#include "core/unique_id.h"
#include "filesystem/filesystem.h"
#include "render/buffer.h"

namespace erwin
{

enum class ShaderType: uint8_t
{
    Vertex = 0,
    TessellationControl = 1,
    TessellationEvaluation = 2,
    Geometry = 3,
    Fragment = 4,
    GLCompute = 5,
};

class Texture2D;

class Shader
{
public:
	Shader(): unique_id_(id::unique_id()) { }
	virtual ~Shader() = default;

	// Initialize shader from glsl source string
	virtual bool init_glsl_string(const std::string& name, const std::string& source) { return false; }
	// Initialize shader from packed GLSL source
	virtual bool init_glsl(const std::string& name, const fs::path& glsl_file) { return false; }
	// Initialize shader from SPIR-V file
	virtual bool init_spirv(const std::string& name, const fs::path& spv_file) { return false; }

	// Use this program
	virtual void bind() const = 0;
	// Stop using this program
	virtual void unbind() const = 0;
	// Get texture slot associated to hash sampler name (uniform name)
	virtual uint32_t get_texture_slot(hash_t sampler) const = 0;
	// Attach a texture to a sampler without having to manipulate solts
	virtual void attach_texture(hash_t sampler, const Texture2D& texture) const = 0;
	// Attach a shader buffer storage
	virtual void attach_shader_storage(const ShaderStorageBuffer& buffer) const = 0;
	// Attach a uniform buffer
	virtual void attach_uniform_buffer(const UniformBuffer& buffer) const = 0;

	// Return program debug name
	inline const std::string& get_name() const { return name_; }
    // Return engine unique id for this object
    inline W_ID get_unique_id() const { return unique_id_; }

	// Factory method for the creation of an API-specific shader from a file path
	static WRef<Shader> create(const std::string& name, const fs::path& filepath);
	// Factory method for the creation of an API-specific shader from a source string
	static WRef<Shader> create(const std::string& name, const std::string& source_string);

protected:
	std::string name_;
    W_ID unique_id_;
};

// The shader bank holds references to multiple shaders and makes them accessible by name
class ShaderBank
{
public:
	// Add an existing shader to the bank
	void add(WRef<Shader> p_shader);
	// Load shader in bank from path (relative to the asset directory)
	void load(const fs::path& path);
	// Load shader in bank from string
	void load(const std::string& name, const std::string& source_string);
	// Get shader by hash name
	const Shader& get(hash_t name) const;
	// Check whether a shader is registered to this name
	bool exists(hash_t name) const;
	// Get number of loaded shaders
	inline std::size_t get_size() const { return bank_.size(); }
	// Get shader index for input name
	inline uint32_t get_index(hash_t name) const { return indices_.at(name); }

private:
	// Helper func for index map update
	void next_index(hash_t hname);

private:
	std::map<hash_t, WRef<Shader>> bank_;
	std::map<hash_t, uint32_t> indices_;
	uint32_t current_index_ = 0;
};

} // namespace erwin