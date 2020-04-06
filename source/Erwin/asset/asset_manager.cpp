#include "asset/asset_manager.h"
#include "asset/texture_atlas.h"
#include "asset/material.h"
#include "memory/arena.h"
#include "render/renderer.h"
#include "render/renderer_2d.h"
#include "debug/logger.h"
#include "EASTL/vector.h"
#include "EASTL/map.h"
#include "core/eastl_new.h" // new overloads needed by EASTL (linker error otherwise)

namespace erwin
{
constexpr std::size_t k_handle_alloc_size = 5 * 2 * sizeof(HandlePoolT<k_max_asset_handles>);

#define FOR_ALL_HANDLES                   \
		DO_ACTION( TextureAtlasHandle )   \
		DO_ACTION( FontAtlasHandle )      \
		DO_ACTION( TextureGroupHandle )   \
		DO_ACTION( MaterialLayoutHandle ) \
		DO_ACTION( MaterialHandle )


struct MaterialWrapper
{
	MaterialHandle material;
	std::string name;
	std::string description;
};

static struct
{
	eastl::vector<TextureAtlas*> texture_atlases_;
	eastl::vector<FontAtlas*> font_atlases_;
	eastl::vector<TextureGroup*> texture_groups_;
	eastl::vector<Material*> materials_;
	eastl::vector<MaterialLayout> material_layouts_;

	eastl::map<hash_t, TextureAtlasHandle> atlas_cache_;
	eastl::map<hash_t, FontAtlasHandle> font_cache_;
	eastl::map<hash_t, TextureGroupHandle> texture_cache_;
	eastl::map<hash_t, MaterialLayoutHandle> layout_cache_;
	eastl::map<hash_t, ShaderHandle> shader_cache_;
	eastl::map<uint64_t, UniformBufferHandle> ubo_cache_;

	eastl::map<hash_t, MaterialWrapper> material_names_;

	LinearArena handle_arena_;
	PoolArena texture_atlas_pool_;
	PoolArena font_atlas_pool_;
	PoolArena texture_group_pool_;
	PoolArena material_pool_;
} s_storage;


// ---------------- PUBLIC API ----------------

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

TextureGroupHandle AssetManager::load_texture_group(const fs::path& filepath, MaterialLayoutHandle layout)
{
	// If filepath is empty, return default invalid handle
	if(filepath.empty())
		return TextureGroupHandle();

	W_ASSERT_FMT(layout.is_valid(), "MaterialLayoutHandle of index %hu is invalid.", layout.index);

	hash_t hname = H_(filepath.string().c_str());
	auto it = s_storage.texture_cache_.find(hname);
	if(it!=s_storage.texture_cache_.end())
		return it->second;

	TextureGroupHandle handle = TextureGroupHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new texture group:" << std::endl;
	DLOG("asset",1) << "TextureGroupHandle: " << WCC('v') << handle.index << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	TextureGroup* tg = W_NEW(TextureGroup, s_storage.texture_group_pool_);
	const MaterialLayout& ml = s_storage.material_layouts_[layout.index]; 
	tg->load(filesystem::get_asset_dir() / filepath, ml);

	// Register group
	s_storage.texture_groups_[handle.index] = tg;
	s_storage.texture_cache_.insert({hname, handle});

	return handle;
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

UniformBufferHandle AssetManager::create_material_data_buffer(uint64_t component_id, uint32_t size)
{
	auto it = s_storage.ubo_cache_.find(component_id);
	if(it!=s_storage.ubo_cache_.end())
		return it->second;

	auto handle = Renderer::create_uniform_buffer("material_data", nullptr, size, UsagePattern::Dynamic);
	s_storage.ubo_cache_.insert({component_id, handle});
	return handle;
}

MaterialLayoutHandle AssetManager::create_material_layout(const std::vector<hash_t>& texture_slots)
{
	hash_t hname = HCOMBINE_(texture_slots);
	auto it = s_storage.layout_cache_.find(hname);
	if(it!=s_storage.layout_cache_.end())
		return it->second;

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

	s_storage.layout_cache_.insert({hname, handle});
	return handle;
}

MaterialHandle AssetManager::create_material(const std::string& name,
									  		 ShaderHandle shader,
									  		 TextureGroupHandle tg,
									  		 UniformBufferHandle ubo,
									  		 size_t data_size)
{
	MaterialHandle handle = MaterialHandle::acquire();
	Material* material = W_NEW(Material, s_storage.material_pool_);
	material->shader = shader;
	material->texture_group = tg;
	material->ubo = ubo;
	material->data_size = data_size;

	s_storage.material_names_.insert({H_(name.c_str()), {handle, name, ""}});
	s_storage.materials_[handle.index] = material;
	Renderer3D::register_material(handle);
	return handle;
}

void AssetManager::visit_materials(MaterialVisitor visit)
{
	for(auto&& [key, wrapper]: s_storage.material_names_)
		if(visit(*s_storage.materials_[wrapper.material.index], wrapper.name, wrapper.description))
			break;
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

void AssetManager::release(ShaderHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "ShaderHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing shader:" << std::endl;

	erase_by_value(s_storage.shader_cache_, handle);
	Renderer::destroy(handle);

	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
}

void AssetManager::release(MaterialLayoutHandle handle)
{
	W_ASSERT_FMT(handle.is_valid(), "MaterialLayoutHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing material layout:" << std::endl;
	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
	erase_by_value(s_storage.layout_cache_, handle);
	handle.release();
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