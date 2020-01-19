#include "asset/asset_manager.h"
#include "asset/texture_atlas.h"
#include "asset/material.h"
#include "memory/arena.h"
#include "render/renderer.h"
#include "render/renderer_2d.h"
#include "debug/logger.h"
#include "EASTL/vector.h"

#include "debug/texture_peek.h"

// #define DEBUG_TEXTURES

namespace erwin
{
constexpr std::size_t k_handle_alloc_size = 4 * 2 * sizeof(HandlePoolT<k_max_asset_handles>);

#define FOR_ALL_HANDLES                 \
		DO_ACTION( TextureAtlasHandle ) \
		DO_ACTION( FontAtlasHandle )    \
		DO_ACTION( TextureGroupHandle ) \
		DO_ACTION( MaterialLayoutHandle )


static struct AssetManagerStorage
{
	eastl::vector<TextureAtlas*> texture_atlases_;
	eastl::vector<FontAtlas*> font_atlases_;
	eastl::vector<TextureGroup*> texture_groups_;
	eastl::vector<MaterialLayout> material_layouts_;

	LinearArena handle_arena_;
	PoolArena texture_atlas_pool_;
	PoolArena font_atlas_pool_;
	PoolArena texture_group_pool_;
} s_storage;


// ---------------- PUBLIC API ----------------

TextureAtlasHandle AssetManager::load_texture_atlas(const fs::path& filepath)
{
	TextureAtlasHandle handle = TextureAtlasHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new texture atlas:" << std::endl;
	DLOG("asset",1) << "TextureAtlasHandle: " << WCC('v') << handle.index << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	TextureAtlas* atlas = W_NEW(TextureAtlas, s_storage.texture_atlas_pool_);
	atlas->load(filesystem::get_asset_dir() / filepath);

#ifdef DEBUG_TEXTURES
	uint32_t pane = TexturePeek::new_pane(filepath.stem().string());
	TexturePeek::register_texture(pane, atlas->texture, "diffuse", false);
#endif

	// Register atlas
	Renderer2D::create_batch(atlas->texture); // TODO: this should be conditional
	s_storage.texture_atlases_[handle.index] = atlas;

	return handle;
}

FontAtlasHandle AssetManager::load_font_atlas(const fs::path& filepath)
{
	FontAtlasHandle handle = FontAtlasHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new font atlas:" << std::endl;
	DLOG("asset",1) << "FontAtlasHandle: " << WCC('v') << handle.index << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	FontAtlas* atlas = W_NEW(FontAtlas, s_storage.font_atlas_pool_);
	atlas->load(filesystem::get_asset_dir() / filepath);

#ifdef DEBUG_TEXTURES
	uint32_t pane = TexturePeek::new_pane(filepath.stem().string());
	TexturePeek::register_texture(pane, atlas->texture, "diffuse", false);
#endif

	// Register atlas
	s_storage.font_atlases_[handle.index] = atlas;

	return handle;
}

TextureGroupHandle AssetManager::load_texture_group(const fs::path& filepath, MaterialLayoutHandle layout)
{
	W_ASSERT_FMT(layout.is_valid(), "MaterialLayoutHandle of index %hu is invalid.", layout.index);

	TextureGroupHandle handle = TextureGroupHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new texture group:" << std::endl;
	DLOG("asset",1) << "TextureGroupHandle: " << WCC('v') << handle.index << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	TextureGroup* tg = W_NEW(TextureGroup, s_storage.texture_group_pool_);
	const MaterialLayout& ml = s_storage.material_layouts_[layout.index]; 
	tg->load(filesystem::get_asset_dir() / filepath, ml);

#ifdef DEBUG_TEXTURES
	uint32_t pane = TexturePeek::new_pane(filepath.stem().string());
	for(uint32_t ii=0; ii<tg->texture_count; ++ii)
	{
		std::string name = "Slot_" + std::to_string(ii);
		TexturePeek::register_texture(pane, tg->textures[ii], name, false);
	}
#endif

	// Register group
	s_storage.texture_groups_[handle.index] = tg;

	return handle;
}

ShaderHandle AssetManager::load_shader(const fs::path& filepath, const std::string& name)
{
	DLOGN("asset") << "[AssetManager] Creating new shader:" << std::endl;

	std::string shader_name = name.empty() ? filepath.stem().string() : name;
	ShaderHandle handle = Renderer::create_shader(filesystem::get_asset_dir() / filepath, shader_name);

	DLOG("asset",1) << "ShaderHandle: " << WCC('v') << handle.index << std::endl;
	return handle;
}

MaterialLayoutHandle AssetManager::create_material_layout(const std::vector<hash_t>& texture_slots)
{
	MaterialLayoutHandle handle = MaterialLayoutHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new material layout:" << std::endl;
	DLOG("asset",1) << "MaterialLayoutHandle: " << WCC('v') << handle.index << std::endl;

	// Sanity check
	uint32_t texture_count = texture_slots.size();
	W_ASSERT_FMT(texture_count <= k_max_texture_slots, "Too many texture slots, expect %u max, got %u instead.", k_max_texture_slots, texture_count);

	// Initialize material layout
	MaterialLayout& ml = s_storage.material_layouts_[handle.index];
	ml.texture_count = texture_count;
	for(uint32_t ii=0; ii<texture_count; ++ii)
		ml.texture_slots[ii] = texture_slots[ii];

	return handle;
}

UniformBufferHandle AssetManager::create_material_data_buffer(uint32_t size)
{
	return Renderer::create_uniform_buffer("material_data", nullptr, size, UsagePattern::Dynamic);
}

void AssetManager::release(TextureAtlasHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "TextureAtlasHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing texture atlas:" << std::endl;
	TextureAtlas* atlas = s_storage.texture_atlases_.at(handle.index);
	atlas->release();
	W_DELETE(atlas, s_storage.texture_atlas_pool_);
	s_storage.texture_atlases_[handle.index] = nullptr;
	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
	handle.release();
}

void AssetManager::release(FontAtlasHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "FontAtlasHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing font atlas:" << std::endl;
	FontAtlas* atlas = s_storage.font_atlases_.at(handle.index);
	atlas->release();
	W_DELETE(atlas, s_storage.font_atlas_pool_);
	s_storage.font_atlases_[handle.index] = nullptr;
	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
	handle.release();
}

void AssetManager::release(TextureGroupHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "TextureGroupHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing texture group:" << std::endl;
	TextureGroup* tg = s_storage.texture_groups_.at(handle.index);
	tg->release();
	W_DELETE(tg, s_storage.texture_group_pool_);
	s_storage.texture_groups_[handle.index] = nullptr;
	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
	handle.release();
}

void AssetManager::release(ShaderHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "ShaderHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing shader:" << std::endl;

	Renderer::destroy(handle);

	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
}

void AssetManager::release(MaterialLayoutHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "MaterialLayoutHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing material layout:" << std::endl;
	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
	handle.release();
}

void AssetManager::release(UniformBufferHandle handle)
{
	Renderer::destroy(handle);
}

// ---------------- PRIVATE API ----------------

void AssetManager::init(memory::HeapArea& area)
{
	s_storage.handle_arena_.init(area, k_handle_alloc_size, "AssetHandles");
	s_storage.texture_atlas_pool_.init(area, sizeof(TextureAtlas) + PoolArena::DECORATION_SIZE, k_max_atlases, "TextureAtlasPool");
	s_storage.font_atlas_pool_.init(area, sizeof(FontAtlas) + PoolArena::DECORATION_SIZE, k_max_font_atlases, "FontAtlasPool");
	s_storage.texture_group_pool_.init(area, sizeof(TextureGroup) + PoolArena::DECORATION_SIZE, k_max_texture_groups, "TextureGroupPool");

	s_storage.texture_atlases_.resize(k_max_atlases, nullptr);
	s_storage.font_atlases_.resize(k_max_font_atlases, nullptr);
	s_storage.texture_groups_.resize(k_max_texture_groups, nullptr);
	s_storage.material_layouts_.resize(k_max_material_layouts);

	// Init handle pools
	#define DO_ACTION( HANDLE_NAME ) HANDLE_NAME::init_pool(s_storage.handle_arena_);
	FOR_ALL_HANDLES
	#undef DO_ACTION

}

void AssetManager::shutdown()
{
	for(TextureAtlas* atlas: s_storage.texture_atlases_)
		if(atlas)
			W_DELETE(atlas, s_storage.texture_atlas_pool_);

	for(FontAtlas* atlas: s_storage.font_atlases_)
		if(atlas)
			W_DELETE(atlas, s_storage.font_atlas_pool_);

	for(TextureGroup* tg: s_storage.texture_groups_)
		if(tg)
			W_DELETE(tg, s_storage.texture_group_pool_);

	// Destroy handle pools
	#define DO_ACTION( HANDLE_NAME ) HANDLE_NAME::destroy_pool(s_storage.handle_arena_);
	FOR_ALL_HANDLES
	#undef DO_ACTION
}

const TextureAtlas& AssetManager::get(TextureAtlasHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "TextureAtlasHandle of index %hu is invalid.", handle.index);
	return *s_storage.texture_atlases_[handle.index];
}

const FontAtlas& AssetManager::get(FontAtlasHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "FontAtlasHandle of index %hu is invalid.", handle.index);
	return *s_storage.font_atlases_[handle.index];
}

const TextureGroup& AssetManager::get(TextureGroupHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "TextureGroupHandle of index %hu is invalid.", handle.index);
	return *s_storage.texture_groups_[handle.index];
}


} // namespace erwin