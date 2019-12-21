#pragma once

#include "asset/handles.h"
#include "render/handles.h"
#include "filesystem/filesystem.h"
#include "memory/memory.hpp"

namespace erwin
{

struct TextureAtlas;
class AssetManager
{
public:
	static TextureAtlasHandle load_texture_atlas(const fs::path& filepath);
	static void release_texture_atlas(TextureAtlasHandle handle);

	static ShaderHandle load_shader(const fs::path& filepath, const std::string& name="");
	static void release_shader(ShaderHandle handle);

private:
	friend class Renderer2D;
	friend class Application;

	static void init(memory::HeapArea& area);
	static void shutdown();
	static const TextureAtlas& get(TextureAtlasHandle handle);
};


} // namespace erwin