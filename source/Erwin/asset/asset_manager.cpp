#include "asset/asset_manager.h"
#include "asset/texture_atlas.h"
#include "asset/material.h"
#include "memory/arena.h"
#include "render/main_renderer.h"
#include "render/renderer_2d.h"
#include "debug/logger.h"
#include "EASTL/vector.h"

namespace erwin
{
constexpr std::size_t k_handle_alloc_size = 2 * 2 * sizeof(HandlePoolT<k_max_asset_handles>);

#define FOR_ALL_HANDLES                 \
		DO_ACTION( TextureAtlasHandle ) \
		DO_ACTION( MaterialHandle )


static struct AssetManagerStorage
{
	eastl::vector<TextureAtlas*> texture_atlases_;
	eastl::vector<Material*> materials_;

	LinearArena handle_arena_;
	PoolArena texture_atlas_pool_;
	PoolArena material_pool_;
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

	// Register atlas
	Renderer2D::create_batch(atlas->texture); // TODO: this should be conditional
	s_storage.texture_atlases_[handle.index] = atlas;

	return handle;
}

MaterialHandle AssetManager::load_material(const fs::path& filepath)
{
	MaterialHandle handle = MaterialHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new material:" << std::endl;
	DLOG("asset",1) << "MaterialHandle: " << WCC('v') << handle.index << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	Material* material = W_NEW(Material, s_storage.material_pool_);
	material->load(filesystem::get_asset_dir() / filepath);

	// Register material
	s_storage.materials_[handle.index] = material;

	return handle;
}

ShaderHandle AssetManager::load_shader(const fs::path& filepath, const std::string& name)
{
	DLOGN("asset") << "[AssetManager] Creating new shader:" << std::endl;

	std::string shader_name = name.empty() ? filepath.stem().string() : name;
	ShaderHandle handle = MainRenderer::create_shader(filesystem::get_asset_dir() / filepath, shader_name);

	DLOG("asset",1) << "ShaderHandle: " << WCC('v') << handle.index << std::endl;
	return handle;
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

void AssetManager::release(MaterialHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "MaterialHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing material:" << std::endl;
	Material* material = s_storage.materials_.at(handle.index);
	material->release();
	W_DELETE(material, s_storage.material_pool_);
	s_storage.materials_[handle.index] = nullptr;
	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
	handle.release();
}

void AssetManager::release(ShaderHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "ShaderHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing shader:" << std::endl;

	MainRenderer::destroy(handle);

	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
}


// ---------------- PRIVATE API ----------------

void AssetManager::init(memory::HeapArea& area)
{
	s_storage.handle_arena_.init(area, k_handle_alloc_size, "AssetHandles");
	s_storage.texture_atlas_pool_.init(area, sizeof(TextureAtlas) + PoolArena::DECORATION_SIZE, k_max_atlases, "AtlasPool");
	s_storage.material_pool_.init(area, sizeof(Material) + PoolArena::DECORATION_SIZE, k_max_materials, "MaterialPool");

	s_storage.texture_atlases_.resize(k_max_atlases, nullptr);
	s_storage.materials_.resize(k_max_materials, nullptr);

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

	for(Material* material: s_storage.materials_)
		if(material)
			W_DELETE(material, s_storage.material_pool_);

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

const Material& AssetManager::get(MaterialHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "MaterialHandle of index %hu is invalid.", handle.index);
	return *s_storage.materials_[handle.index];
}


} // namespace erwin