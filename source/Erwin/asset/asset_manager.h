#pragma once

#include "asset/handles.h"
#include "render/handles.h"
#include "filesystem/filesystem.h"
#include "memory/memory.hpp"

namespace erwin
{

struct TextureAtlas;
struct Material;
class AssetManager
{
public:
	static TextureAtlasHandle load_texture_atlas(const fs::path& filepath);
	static MaterialHandle load_material(const fs::path& filepath);
	static ShaderHandle load_shader(const fs::path& filepath, const std::string& name="");
	
	static void release(TextureAtlasHandle handle);
	static void release(MaterialHandle handle);
	static void release(ShaderHandle handle);

private:
	friend class Renderer2D;
	friend class Application;

	static void init(memory::HeapArea& area);
	static void shutdown();
	static const TextureAtlas& get(TextureAtlasHandle handle);
	static const Material& get(MaterialHandle handle);
};


} // namespace erwin