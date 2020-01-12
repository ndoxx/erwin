#pragma once

#include <vector>
#include "asset/handles.h"
#include "render/handles.h"
#include "filesystem/filesystem.h"
#include "memory/memory.hpp"

namespace erwin
{

struct TextureAtlas;
struct FontAtlas;
struct TextureGroup;
class AssetManager
{
public:
	static TextureAtlasHandle load_texture_atlas(const fs::path& filepath);
	static FontAtlasHandle load_font_atlas(const fs::path& filepath);
	static TextureGroupHandle load_texture_group(const fs::path& filepath, MaterialLayoutHandle layout);
	static ShaderHandle load_shader(const fs::path& filepath, const std::string& name="");

	static MaterialLayoutHandle create_material_layout(const std::vector<hash_t>& texture_slots);
	static UniformBufferHandle create_material_data_buffer(uint32_t size);
	
	static void release(TextureAtlasHandle handle);
	static void release(FontAtlasHandle handle);
	static void release(TextureGroupHandle handle);
	static void release(ShaderHandle handle);
	static void release(MaterialLayoutHandle handle);
	static void release(UniformBufferHandle handle);

private:
	friend class Renderer2D;
	friend class Application;
	friend class ForwardRenderer;

	static void init(memory::HeapArea& area);
	static void shutdown();
	static const TextureAtlas& get(TextureAtlasHandle handle);
	static const FontAtlas& get(FontAtlasHandle handle);
	static const TextureGroup& get(TextureGroupHandle handle);
};


} // namespace erwin