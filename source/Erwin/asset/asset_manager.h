#pragma once

#include <vector>
#include <functional>
#include "asset/handles.h"
#include "asset/material.h"
#include "render/handles.h"
#include "render/renderer_3d.h"
#include "filesystem/filesystem.h"
#include "memory/memory.hpp"
#include "ctti/type_id.hpp"

namespace erwin
{

/*
	TODO: [ ] Code universal paths that could point to an engine resource or an
	application resource.
*/

struct TextureAtlas;
struct FontAtlas;
class AssetManager
{
public:
	using MaterialVisitor = std::function<bool(const Material&, const std::string&, const std::string&)>;

	// Cached queries
	static TextureAtlasHandle load_texture_atlas(const fs::path& filepath);
	static FontAtlasHandle load_font_atlas(const fs::path& filepath);
	static TextureHandle load_image(const fs::path& filepath, uint32_t& width, uint32_t& height, bool engine_path=false);
	static ShaderHandle load_shader(const fs::path& filepath, const std::string& name="");
	static ShaderHandle load_system_shader(const fs::path& filepath, const std::string& name="");

	static TextureGroup load_texture_group(const fs::path& filepath);

	// Create a material from diverse resource handles
	static const Material& create_material(const std::string& name,
										   const TextureGroup& tg,
										   ShaderHandle shader,
										   UniformBufferHandle ubo,
										   uint32_t data_size,
										   bool is_public=true);

	// Create a complete material from file paths to its component assets
	template <typename ComponentT>
	static inline const Material& create_material(const std::string& name,
												  const fs::path& shader_path,
												  const fs::path& texture_group_path="")
	{
		using MaterialData = typename ComponentT::MaterialData;

		ShaderHandle shader     = load_shader(shader_path);
		TextureGroup tg         = load_texture_group(texture_group_path);
		UniformBufferHandle ubo = create_material_data_buffer(ctti::type_id<ComponentT>().hash(), sizeof(MaterialData));

		return create_material(name, tg, shader, ubo, sizeof(MaterialData));
	}

	static const Material& get_material(hash_t arch_name);
	static const std::string& get_material_name(hash_t arch_name);

	static void visit_materials(MaterialVisitor visit);

	static void release(TextureAtlasHandle handle);
	static void release(FontAtlasHandle handle);
	static void release(ShaderHandle handle);
	static void release(UniformBufferHandle handle);

	static void release(TextureGroup tg);
	static void release(hash_t material);

private:
	friend class Renderer2D;
	friend class Renderer3D;
	friend class Application;

	static void init(memory::HeapArea& area);
	static void shutdown();
	static const TextureAtlas& get(TextureAtlasHandle handle);
	static const FontAtlas& get(FontAtlasHandle handle);

	static UniformBufferHandle create_material_data_buffer(uint64_t id, uint32_t size);
};

} // namespace erwin