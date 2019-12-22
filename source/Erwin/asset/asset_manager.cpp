#include "asset/asset_manager.h"
#include "asset/texture_atlas.h"
#include "memory/arena.h"
#include "render/main_renderer.h"
#include "render/renderer_2d.h"
#include "debug/logger.h"

namespace erwin
{
constexpr std::size_t k_handle_alloc_size = 1 * 2 * sizeof(HandlePoolT<k_max_asset_handles>);


#define FOR_ALL_HANDLES                 \
		DO_ACTION( TextureAtlasHandle )


static struct AssetManagerStorage
{
	std::vector<TextureAtlas*> texture_atlases_;

	LinearArena handle_arena_;
	PoolArena texture_atlas_pool_;
} s_storage;


// ---------------- PUBLIC API ----------------

TextureAtlasHandle AssetManager::load_texture_atlas(const fs::path& filepath)
{
	DLOGN("asset") << "[AssetManager] Creating new texture atlas:" << std::endl;

	TextureAtlasHandle handle = TextureAtlasHandle::acquire();
	TextureAtlas* atlas = W_NEW(TextureAtlas, s_storage.texture_atlas_pool_);
	atlas->load(filesystem::get_asset_dir() / filepath);

	// * Register atlas
	ImageFormat format;
	switch(atlas->descriptor.texture_compression)
	{
		case TextureCompression::None: format = ImageFormat::SRGB_ALPHA; break;
		case TextureCompression::DXT1: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1; break;
		case TextureCompression::DXT5: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5; break;
	}
	uint8_t filter = MAG_NEAREST | MIN_NEAREST;
	// uint8_t filter = MAG_NEAREST | MIN_LINEAR_MIPMAP_NEAREST;
	// uint8_t filter = MAG_LINEAR | MIN_NEAREST_MIPMAP_NEAREST;

	atlas->texture = MainRenderer::create_texture_2D(Texture2DDescriptor{atlas->descriptor.texture_width,
								  					 				     atlas->descriptor.texture_height,
								  					 				     atlas->descriptor.texture_blob,
								  					 				     format,
								  					 				     filter});
	// TODO: this should be conditional
	Renderer2D::create_batch(atlas->texture);

	s_storage.texture_atlases_[handle.index] = atlas;

	DLOGI << "handle: " << WCC('v') << handle.index << std::endl;

	return handle;
}

void AssetManager::release(TextureAtlasHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "TextureAtlasHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing texture atlas:" << std::endl;
	TextureAtlas* atlas = s_storage.texture_atlases_.at(handle.index);
	W_DELETE(atlas, s_storage.texture_atlas_pool_);
	s_storage.texture_atlases_[handle.index] = nullptr;
	DLOGI << "handle: " << WCC('v') << handle.index << std::endl;
}

ShaderHandle AssetManager::load_shader(const fs::path& filepath, const std::string& name)
{
	DLOGN("asset") << "[AssetManager] Creating new shader:" << std::endl;

	std::string shader_name = name.empty() ? filepath.stem().string() : name;
	ShaderHandle handle = MainRenderer::create_shader(filesystem::get_asset_dir() / filepath, shader_name);

	DLOGI << "handle: " << WCC('v') << handle.index << std::endl;
	return handle;
}

void AssetManager::release(ShaderHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "ShaderHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing shader:" << std::endl;

	MainRenderer::destroy(handle);

	DLOGI << "handle: " << WCC('v') << handle.index << std::endl;
}


// ---------------- PRIVATE API ----------------

void AssetManager::init(memory::HeapArea& area)
{
	s_storage.handle_arena_.init(area.require_block(k_handle_alloc_size));
	s_storage.texture_atlas_pool_.init(area.require_pool_block<PoolArena>(sizeof(TextureAtlas), k_max_atlases), sizeof(TextureAtlas), k_max_atlases, PoolArena::DECORATION_SIZE);

	s_storage.texture_atlases_.resize(k_max_atlases, nullptr);

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

	// Destroy handle pools
	#define DO_ACTION( HANDLE_NAME ) HANDLE_NAME::destroy_pool(s_storage.handle_arena_);
	FOR_ALL_HANDLES
	#undef DO_ACTION
}

const TextureAtlas& AssetManager::get(TextureAtlasHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "TextureAtlasHandle of index %hu is invalid.", handle.index);
	return *s_storage.texture_atlases_.at(handle.index);
}


} // namespace erwin