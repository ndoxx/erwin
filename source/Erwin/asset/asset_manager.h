#pragma once

#include <vector>
#include <functional>
#include "asset/handles.h"
#include "asset/material.h"
#include "render/handles.h"
#include "render/texture_common.h"
#include "entity/component_PBR_material.h"
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

	// Proceduraly generated assets for the editor and debug purposes
	// type can be "dashed"_h, "grid"_h, "checkerboard"_h, "white"_h or "red"_h
	static TextureHandle create_debug_texture(hash_t type, uint32_t size_px);

	// Loading stuff
	static TextureAtlasHandle load_texture_atlas(const fs::path& filepath);
	static FontAtlasHandle load_font_atlas(const fs::path& filepath);
	static TextureHandle load_image(const fs::path& filepath, Texture2DDescriptor& descriptor);
	static ShaderHandle load_shader(const fs::path& filepath, const std::string& name="");
	static const ComponentPBRMaterial& load_PBR_material(const fs::path& tom_path);

	static UniformBufferHandle create_material_data_buffer(uint64_t id, uint32_t size);
	template <typename ComponentT>
	static inline UniformBufferHandle create_material_data_buffer()
	{
		using MaterialData = typename ComponentT::MaterialData;
		return create_material_data_buffer(ctti::type_id<ComponentT>().hash(), sizeof(MaterialData));
	}

	static void release(TextureAtlasHandle handle);
	static void release(FontAtlasHandle handle);
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
};

} // namespace erwin