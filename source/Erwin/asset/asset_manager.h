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
	using MaterialVisitor = std::function<bool(MaterialHandle, const std::string&, const std::string&)>;

	// Cached queries
	static TextureAtlasHandle load_texture_atlas(const fs::path& filepath);
	static FontAtlasHandle load_font_atlas(const fs::path& filepath);
	static TextureGroupHandle load_texture_group(const fs::path& filepath);
	static ShaderHandle load_shader(const fs::path& filepath, const std::string& name="");
	static UniformBufferHandle create_material_data_buffer(uint64_t id, uint32_t size);

	// Create a material from diverse resource handles
	static MaterialHandle create_material(const std::string& name,
										  ShaderHandle shader,
										  TextureGroupHandle tg,
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
		TextureGroupHandle tg   = load_texture_group(texture_group_path);
		UniformBufferHandle ubo = create_material_data_buffer(ID, sizeof(MaterialData));

		return create_material(name, shader, tg, ubo, sizeof(MaterialData));
	}

	static const std::string& get_name(MaterialHandle handle);

	static void visit_materials(MaterialVisitor visit);

	static void release(TextureAtlasHandle handle);
	static void release(FontAtlasHandle handle);
	static void release(TextureGroupHandle handle);
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
	static const TextureGroup& get(TextureGroupHandle handle);
	static const Material& get(MaterialHandle handle);
};

} // namespace erwin