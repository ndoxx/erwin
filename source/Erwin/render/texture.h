#pragma once

#include <map>

#include "core/core.h"
#include "core/unique_id.h"
#include "filesystem/filesystem.h"
#include "render/texture_common.h"
#include "memory/arena.h"

namespace erwin
{

class Texture
{
public:
	Texture(): unique_id_(id::unique_id()) {}
	virtual ~Texture() = default;

	virtual uint32_t get_width() const = 0;
	virtual uint32_t get_height() const = 0;
	virtual uint32_t get_mips() const = 0;

	virtual void bind(uint32_t slot = 0) const = 0;
	virtual void unbind() const = 0;

	virtual void generate_mipmaps() const = 0;
	virtual std::pair<uint8_t*, size_t> read_pixels() const { return {nullptr,0}; }
	virtual void* get_native_handle() = 0;

    inline W_ID get_unique_id() const { return unique_id_; }

protected:
    W_ID unique_id_;
};

class Texture2D: public Texture
{
public:
	virtual ~Texture2D() = default;

	// Create a 2D texture from a file
	static WRef<Texture2D> create(const fs::path& filepath);
	// Create a 2D texture from descriptor
	static WRef<Texture2D> create(const Texture2DDescriptor& descriptor);
	// static Texture2D* create(PoolArena& arena, const Texture2DDescriptor& descriptor);
};

class Cubemap: public Texture
{
public:
	virtual ~Cubemap() = default;

	// Create a cubemap texture from descriptor
	static WRef<Cubemap> create(const CubemapDescriptor& descriptor);
};

} // namespace erwin