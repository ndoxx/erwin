#pragma once

#include <vector>
#include <functional>
#include "asset/handles.h"
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
struct Material;
class AssetManager
{
public:
	using MaterialVisitor = std::function<bool(const Material&, const std::string&, const std::string&)>;

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

	static MaterialHandle create_material(const std::string& name,
										  ShaderHandle shader,
										  TextureGroupHandle tg,
										  UniformBufferHandle ubo,
										  size_t data_size);

	// Create a complete material from file paths to multiple assets
	template <typename ComponentT>
	static inline MaterialHandle create_material(const std::string& name,
												 const std::vector<hash_t>& layout_list,
												 const fs::path& shader_path,
												 const fs::path& texture_group_path)
	{
		MaterialLayoutHandle layout = create_material_layout(layout_list);
		ShaderHandle shader         = load_shader(shader_path);
		TextureGroupHandle tg       = load_texture_group(texture_group_path, layout);
		UniformBufferHandle ubo     = create_material_data_buffer<ComponentT>();

		return create_material(name, shader, tg, ubo, sizeof(typename ComponentT::MaterialData));
	}
	
	template <typename ComponentT>
	static inline MaterialHandle create_material(const std::string& name,
												 const fs::path& shader_path)
	{
		ShaderHandle shader     = load_shader(shader_path);
		UniformBufferHandle ubo = create_material_data_buffer<ComponentT>();

		return create_material(name, shader, {}, ubo, sizeof(typename ComponentT::MaterialData));
	}

	static void visit_materials(MaterialVisitor visit);

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
	static const Material& get(MaterialHandle handle);
};

} // namespace erwin