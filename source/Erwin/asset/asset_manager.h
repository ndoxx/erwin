#pragma once

#include <vector>
#include "asset/handles.h"
#include "asset/material.h"
#include "render/handles.h"
#include "render/renderer_3d.h"
#include "filesystem/filesystem.h"
#include "memory/memory.hpp"
#include "ctti/type_id.hpp"

namespace erwin
{

struct TextureAtlas;
struct FontAtlas;
struct TextureGroup;
class AssetManager
{
public:
	// Cached queries
	static TextureAtlasHandle load_texture_atlas(const fs::path& filepath);
	static FontAtlasHandle load_font_atlas(const fs::path& filepath);
	static TextureGroupHandle load_texture_group(const fs::path& filepath, MaterialLayoutHandle layout);
	static ShaderHandle load_shader(const fs::path& filepath, const std::string& name="");

	template <typename ComponentT>
	static inline UniformBufferHandle create_material_data_buffer()
	{
		static constexpr uint64_t ID = ctti::type_id<ComponentT>().hash();
		using MaterialData = typename ComponentT::MaterialData;
		return create_material_data_buffer(ID, sizeof(MaterialData));
	}
	static UniformBufferHandle create_material_data_buffer(uint64_t component_id, uint32_t size);
	static MaterialLayoutHandle create_material_layout(const std::vector<hash_t>& texture_slots);

	// Create a complete material from file paths to multiple assets
	template <typename ComponentT>
	static inline Material create_material(const std::vector<hash_t>& layout_list,
										   const fs::path& shader_path,
										   const fs::path& texture_group_path)
	{
		MaterialLayoutHandle layout = create_material_layout(layout_list);
		ShaderHandle shader         = load_shader(shader_path);
		TextureGroupHandle tg       = load_texture_group(texture_group_path, layout);
		UniformBufferHandle ubo     = create_material_data_buffer<ComponentT>();

		Material material {shader, tg, ubo};
		Renderer3D::register_material(material);
		return material;
	}
	
	template <typename ComponentT>
	static inline Material create_material(const fs::path& shader_path)
	{
		ShaderHandle shader     = load_shader(shader_path);
		UniformBufferHandle ubo = create_material_data_buffer<ComponentT>();

		Material material {shader, {}, ubo};
		Renderer3D::register_material(material);
		return material;
	}

	static void release(TextureAtlasHandle handle);
	static void release(FontAtlasHandle handle);
	static void release(TextureGroupHandle handle);
	static void release(ShaderHandle handle);
	static void release(MaterialLayoutHandle handle);
	static void release(UniformBufferHandle handle);

private:
	friend class Renderer2D;
	friend class Renderer3D;
	friend class Application;

	static void init(memory::HeapArea& area);
	static void shutdown();
	static const TextureAtlas& get(TextureAtlasHandle handle);
	static const FontAtlas& get(FontAtlasHandle handle);
	static const TextureGroup& get(TextureGroupHandle handle);
};

} // namespace erwin