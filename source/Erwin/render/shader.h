#pragma once

#include <istream>
#include <string>
#include <memory>
#include <unordered_map>

#include "core/wtypes.h"
#include "core/file_system.h"
#include "render/buffer.h"

namespace erwin
{

enum class ShaderType: uint8_t
{
	None = 0,
	Vertex,
	Geometry,
	Fragment,
};

class Texture2D;
struct ShaderParameters
{
	void set_texture_slot(hash_t sampler_name, std::shared_ptr<Texture2D> texture);

	std::unordered_map<hash_t, std::shared_ptr<Texture2D>> texture_slots;
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
	// Get texture slot associated to hash sampler name (uniform name)
	virtual uint32_t get_texture_slot(hash_t sampler) const = 0;
	// Attach a shader buffer storage
	virtual void attach_shader_storage(const ShaderStorageBuffer& buffer, const std::string& name) const = 0;

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
	// Get number of loaded shaders
	inline std::size_t get_size() const { return bank_.size(); }
	// Get shader index for input name
	inline uint32_t get_index(hash_t name) const { return indices_.at(name); }

private:
	// Helper func for index map update
	void next_index(hash_t hname);

private:
	std::unordered_map<hash_t, std::shared_ptr<Shader>> bank_;
	std::unordered_map<hash_t, uint32_t> indices_;
	uint32_t current_index_ = 0;
};

} // namespace erwin