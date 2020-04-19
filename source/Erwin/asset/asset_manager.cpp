#include "asset/asset_manager.h"
#include "asset/texture_atlas.h"
#include "asset/material.h"
#include "filesystem/hdr_file.h"
#include "memory/arena.h"
#include "render/renderer.h"
#include "render/renderer_2d.h"
#include "render/common_geometry.h"
#include "debug/logger.h"
#include "EASTL/vector.h"
#include "EASTL/map.h"
#include "core/eastl_new.h" // new overloads needed by EASTL (linker error otherwise)

namespace erwin
{
constexpr std::size_t k_handle_alloc_size = 2*(sizeof(HandlePoolT<k_max_atlases>)
										  +    sizeof(HandlePoolT<k_max_font_atlases>)
										  +    sizeof(HandlePoolT<k_max_texture_groups>)
										  +    sizeof(HandlePoolT<k_max_materials>));

#define FOR_ALL_HANDLES                   \
		DO_ACTION( TextureAtlasHandle )   \
		DO_ACTION( FontAtlasHandle )      \
		DO_ACTION( TextureGroupHandle )   \
		DO_ACTION( MaterialHandle )


struct MaterialDescriptor
{
	MaterialHandle material;
	bool is_public;
	std::string name;
	std::string description;
};

static struct
{
	eastl::vector<TextureAtlas*> texture_atlases_;
	eastl::vector<FontAtlas*> font_atlases_;
	eastl::vector<TextureGroup*> texture_groups_;
	eastl::vector<Material*> materials_;

	eastl::map<hash_t, TextureAtlasHandle> atlas_cache_;
	eastl::map<hash_t, FontAtlasHandle> font_cache_;
	eastl::map<hash_t, TextureGroupHandle> texture_cache_;
	eastl::map<hash_t, ShaderHandle> shader_cache_;
	eastl::map<uint64_t, UniformBufferHandle> ubo_cache_;

	eastl::vector<MaterialDescriptor> material_descriptors_;

	LinearArena handle_arena_;
	PoolArena texture_atlas_pool_;
	PoolArena font_atlas_pool_;
	PoolArena texture_group_pool_;
	PoolArena material_pool_;
} s_storage;


// ---------------- PUBLIC API ----------------

// TODO: Cache lookup before creating any resource
TextureAtlasHandle AssetManager::load_texture_atlas(const fs::path& filepath)
{
	hash_t hname = H_(filepath.string().c_str());
	auto it = s_storage.atlas_cache_.find(hname);
	if(it!=s_storage.atlas_cache_.end())
		return it->second;

	TextureAtlasHandle handle = TextureAtlasHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new texture atlas:" << std::endl;
	DLOG("asset",1) << "TextureAtlasHandle: " << WCC('v') << handle.index << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	TextureAtlas* atlas = W_NEW(TextureAtlas, s_storage.texture_atlas_pool_);
	atlas->load(filesystem::get_asset_dir() / filepath);

	// Register atlas
	Renderer2D::create_batch(atlas->texture); // TODO: this should be conditional
	s_storage.texture_atlases_[handle.index] = atlas;
	s_storage.atlas_cache_.insert({hname, handle});

	return handle;
}

FontAtlasHandle AssetManager::load_font_atlas(const fs::path& filepath)
{
	hash_t hname = H_(filepath.string().c_str());
	auto it = s_storage.font_cache_.find(hname);
	if(it!=s_storage.font_cache_.end())
		return it->second;

	FontAtlasHandle handle = FontAtlasHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new font atlas:" << std::endl;
	DLOG("asset",1) << "FontAtlasHandle: " << WCC('v') << handle.index << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	FontAtlas* atlas = W_NEW(FontAtlas, s_storage.font_atlas_pool_);
	atlas->load(filesystem::get_asset_dir() / filepath);

	// Register atlas
	s_storage.font_atlases_[handle.index] = atlas;
	s_storage.font_cache_.insert({hname, handle});

	return handle;
}

TextureGroupHandle AssetManager::load_texture_group(const fs::path& filepath)
{
	// If filepath is empty, return default invalid handle
	if(filepath.empty())
		return TextureGroupHandle();

	hash_t hname = H_(filepath.string().c_str());
	auto it = s_storage.texture_cache_.find(hname);
	if(it!=s_storage.texture_cache_.end())
		return it->second;

	TextureGroupHandle handle = TextureGroupHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new texture group:" << std::endl;
	DLOG("asset",1) << "TextureGroupHandle: " << WCC('v') << handle.index << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	TextureGroup* tg = W_NEW(TextureGroup, s_storage.texture_group_pool_);
	tg->load(filesystem::get_asset_dir() / filepath, nullptr); // TODO: get a layout and check shader compatibility

	// Register group
	s_storage.texture_groups_[handle.index] = tg;
	s_storage.texture_cache_.insert({hname, handle});

	return handle;
}

TextureHandle AssetManager::load_hdr(const fs::path& filepath, uint32_t& height)
{
	DLOGN("asset") << "[AssetManager] Loading HDR file:" << std::endl;
	DLOGI << WCC('p') << filepath << WCC(0) << std::endl;
	
	fs::path fullpath = filesystem::get_asset_dir() / filepath;

	// Sanity check
	if(!fs::exists(fullpath))
	{
		DLOGW("asset") << "[AssetManager] File does not exist. Returning invalid handle." << std::endl;
		return TextureHandle();
	}

	if(filepath.extension().string().compare(".hdr"))
	{
		DLOGW("asset") << "[AssetManager] File is not a valid HDR file. Returning invalid handle." << std::endl;
		return TextureHandle();
	}

	// Load HDR file
	hdr::HDRDescriptor desc { fullpath };
	hdr::read_hdr(desc);

	DLOGI << "Width:    " << WCC('v') << desc.width << std::endl;
	DLOGI << "Height:   " << WCC('v') << desc.height << std::endl;
	DLOGI << "Channels: " << WCC('v') << desc.channels << std::endl;

	// Sanity check
	height = desc.height;
	if(2*desc.height != desc.width)
	{
		DLOGW("asset") << "[AssetManager] HDR file must be in 2:1 format (width = 2 * height) for optimal results." << std::endl;
		height = std::min(desc.height, desc.width);
	}

	// Create texture
	return Renderer::create_texture_2D(Texture2DDescriptor{desc.width,
								  					 	   desc.height,
								  					 	   desc.data,
								  					 	   ImageFormat::RGB32F,
								  					 	   MIN_LINEAR | MAG_LINEAR,
								  					 	   TextureWrap::REPEAT,
								  				   		   TF_MUST_FREE}); // Let the renderer free the resources once the texture is loaded
}

ShaderHandle AssetManager::load_shader(const fs::path& filepath, const std::string& name)
{
	hash_t hname = H_(filepath.string().c_str());
	auto it = s_storage.shader_cache_.find(hname);
	if(it!=s_storage.shader_cache_.end())
		return it->second;

	DLOGN("asset") << "[AssetManager] Creating new shader:" << std::endl;

	std::string shader_name = name.empty() ? filepath.stem().string() : name;
	ShaderHandle handle = Renderer::create_shader(filesystem::get_asset_dir() / filepath, shader_name);
	DLOG("asset",1) << "ShaderHandle: " << WCC('v') << handle.index << std::endl;
	s_storage.shader_cache_.insert({hname, handle});

	return handle;
}

ShaderHandle AssetManager::load_system_shader(const fs::path& filepath, const std::string& name)
{
	hash_t hname = H_(filepath.string().c_str());
	auto it = s_storage.shader_cache_.find(hname);
	if(it!=s_storage.shader_cache_.end())
		return it->second;

	DLOGN("asset") << "[AssetManager] Creating new shader:" << std::endl;

	std::string shader_name = name.empty() ? filepath.stem().string() : name;
	ShaderHandle handle = Renderer::create_shader(filesystem::get_system_asset_dir() / filepath, shader_name);
	DLOG("asset",1) << "ShaderHandle: " << WCC('v') << handle.index << std::endl;
	s_storage.shader_cache_.insert({hname, handle});

	return handle;
}

UniformBufferHandle AssetManager::create_material_data_buffer(uint64_t component_id, uint32_t size)
{
	auto it = s_storage.ubo_cache_.find(component_id);
	if(it!=s_storage.ubo_cache_.end())
		return it->second;

	auto handle = Renderer::create_uniform_buffer("material_data", nullptr, size, UsagePattern::Dynamic);
	s_storage.ubo_cache_.insert({component_id, handle});
	return handle;
}

MaterialHandle AssetManager::create_material(const std::string& name,
									  		 ShaderHandle shader,
									  		 TextureGroupHandle tg,
									  		 UniformBufferHandle ubo,
									  		 uint32_t data_size,
									  		 bool is_public)
{
	MaterialHandle handle = MaterialHandle::acquire();
	s_storage.materials_[handle.index] = W_NEW(Material, s_storage.material_pool_) {shader, tg, ubo, data_size};
	s_storage.material_descriptors_[handle.index] = {handle, is_public, name, ""};
	Renderer3D::register_shader(handle);
	return handle;
}

const std::string& AssetManager::get_name(MaterialHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "MaterialHandle of index %hu is invalid.", handle.index);
	return s_storage.material_descriptors_[handle.index].name;
}

void AssetManager::visit_materials(MaterialVisitor visit)
{
	for(size_t ii=0; ii<k_max_materials; ++ii)
	{
		if(s_storage.materials_[ii] == nullptr)
			continue;
		const auto& desc = s_storage.material_descriptors_[ii];
		if(!desc.is_public)
			continue;
		if(visit(desc.material, desc.name, desc.description))
			break;
	}
}

template <typename KeyT, typename HandleT>
static void erase_by_value(eastl::map<KeyT, HandleT>& cache, HandleT handle)
{
	auto it = cache.begin();
	for(; it!=cache.end(); ++it)
    	if(it->second == handle)
    		break;
	cache.erase(it);
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

	erase_by_value(s_storage.atlas_cache_, handle);
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
	
	erase_by_value(s_storage.font_cache_, handle);
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
	
	erase_by_value(s_storage.texture_cache_, handle);
	handle.release();
}

void AssetManager::release(MaterialHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "MaterialHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing material:" << std::endl;
	Material* mat = s_storage.materials_.at(handle.index);
	W_DELETE(mat, s_storage.material_pool_);
	s_storage.materials_[handle.index] = nullptr;
	s_storage.material_descriptors_[handle.index] = {{},false,"",""};
	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
	
	handle.release();
}

void AssetManager::release(ShaderHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "ShaderHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing shader:" << std::endl;

	erase_by_value(s_storage.shader_cache_, handle);
	Renderer::destroy(handle);

	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
}

void AssetManager::release(UniformBufferHandle handle)
{
	erase_by_value(s_storage.ubo_cache_, handle);
	Renderer::destroy(handle);
}

// ---------------- PRIVATE API ----------------

void AssetManager::init(memory::HeapArea& area)
{
	s_storage.handle_arena_.init(area, k_handle_alloc_size, "AssetHandles");
	s_storage.texture_atlas_pool_.init(area, sizeof(TextureAtlas) + PoolArena::DECORATION_SIZE, k_max_atlases, "TextureAtlasPool");
	s_storage.font_atlas_pool_.init(area, sizeof(FontAtlas) + PoolArena::DECORATION_SIZE, k_max_font_atlases, "FontAtlasPool");
	s_storage.texture_group_pool_.init(area, sizeof(TextureGroup) + PoolArena::DECORATION_SIZE, k_max_texture_groups, "TextureGroupPool");
	s_storage.material_pool_.init(area, sizeof(Material) + PoolArena::DECORATION_SIZE, k_max_materials, "MaterialPool");

	s_storage.texture_atlases_.resize(k_max_atlases, nullptr);
	s_storage.font_atlases_.resize(k_max_font_atlases, nullptr);
	s_storage.texture_groups_.resize(k_max_texture_groups, nullptr);
	s_storage.materials_.resize(k_max_materials, nullptr);
	s_storage.material_descriptors_.resize(k_max_materials, {{},false,"",""});

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

	for(Material* mat: s_storage.materials_)
		if(mat)
			W_DELETE(mat, s_storage.material_pool_);

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

const Material& AssetManager::get(MaterialHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "MaterialHandle of index %hu is invalid.", handle.index);
	return *s_storage.materials_[handle.index];
}


} // namespace erwin