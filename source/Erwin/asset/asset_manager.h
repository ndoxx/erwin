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
	TODO: [/] Remove TextureGroupHandle and MaterialHandle
			  -> User should be free to manipulate the objects directly
			  -> However, we need a way for multiple objects to refer to the same material archetype
			     with possible data overrides
*/

struct TextureAtlas;
struct FontAtlas;
class AssetManager
{
public:
	using MaterialVisitor = std::function<bool(MaterialHandle, const std::string&, const std::string&)>;

	// Cached queries
	static TextureAtlasHandle load_texture_atlas(const fs::path& filepath);
	static FontAtlasHandle load_font_atlas(const fs::path& filepath);
	static TextureHandle load_hdr(const fs::path& filepath, uint32_t& height);
	static ShaderHandle load_shader(const fs::path& filepath, const std::string& name="");
	static ShaderHandle load_system_shader(const fs::path& filepath, const std::string& name="");
	static UniformBufferHandle create_material_data_buffer(uint64_t id, uint32_t size);
	
	static TextureGroup load_texture_group(const fs::path& filepath);

	// Create a material from diverse resource handles
	static MaterialHandle create_material(const std::string& name,
										  const TextureGroup& tg,
										  ShaderHandle shader,
										  UniformBufferHandle ubo,
										  uint32_t data_size,
										  bool is_public=true);

	// Create a complete material from file paths to its component assets
	template <typename ComponentT>
	static inline MaterialHandle create_material(const std::string& name,
												 const fs::path& shader_path,
												 const fs::path& texture_group_path="")
	{
		static constexpr uint64_t ID = ctti::type_id<ComponentT>().hash();
		using MaterialData = typename ComponentT::MaterialData;

		ShaderHandle shader     = load_shader(shader_path);
		TextureGroup tg         = load_texture_group(texture_group_path);
		UniformBufferHandle ubo = create_material_data_buffer(ID, sizeof(MaterialData));

		return create_material(name, tg, shader, ubo, sizeof(MaterialData));
	}

	static const std::string& get_name(MaterialHandle handle);

	static void visit_materials(MaterialVisitor visit);

	static void release(TextureAtlasHandle handle);
	static void release(FontAtlasHandle handle);
	static void release(TextureGroup tg);
	static void release(MaterialHandle handle);
	static void release(ShaderHandle handle);
	static void release(UniformBufferHandle handle);

private:
	friend class Renderer2D;
	friend class Renderer3D;
	friend class Application;

	static void init(memory::HeapArea& area);
	static void shutdown();
	static const TextureAtlas& get(TextureAtlasHandle handle);
	static const FontAtlas& get(FontAtlasHandle handle);
	static const Material& get(MaterialHandle handle);
};

} // namespace erwin