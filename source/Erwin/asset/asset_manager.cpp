#include "asset/asset_manager.h"
#include "asset/texture_atlas.h"
#include "asset/material.h"
#include "filesystem/image_file.h"
#include "filesystem/tom_file.h"
#include "memory/arena.h"
#include "render/renderer.h"
#include "render/renderer_2d.h"
#include "render/renderer_3d.h"
#include "render/common_geometry.h"
#include "debug/logger.h"
#include "EASTL/vector.h"
#include "EASTL/map.h"
#include "core/eastl_new.h" // new overloads needed by EASTL (linker error otherwise)
#include "core/intern_string.h"

namespace erwin
{
constexpr std::size_t k_handle_alloc_size = 2*(sizeof(HandlePoolT<k_max_atlases>)
										  +    sizeof(HandlePoolT<k_max_font_atlases>));

#define FOR_ALL_HANDLES                   \
		DO_ACTION( TextureAtlasHandle )   \
		DO_ACTION( FontAtlasHandle )


struct MaterialDescriptor
{
	bool is_public;
	std::string name;
	std::string description;
};

static struct
{
	eastl::vector<TextureAtlas*> texture_atlases_;
	eastl::vector<FontAtlas*> font_atlases_;

	eastl::map<hash_t, TextureAtlasHandle> atlas_cache_;
	eastl::map<hash_t, FontAtlasHandle> font_cache_;
	eastl::map<hash_t, ShaderHandle> shader_cache_;
	eastl::map<uint64_t, UniformBufferHandle> ubo_cache_;

	eastl::map<hash_t, MaterialDescriptor> material_descriptors_;
	eastl::map<hash_t, Material> materials_;

	LinearArena handle_arena_;
	PoolArena texture_atlas_pool_;
	PoolArena font_atlas_pool_;
} s_storage;


static ImageFormat select_image_format(uint8_t channels, TextureCompression compression, bool srgb)
{
	if(channels==4)
	{
		switch(compression)
		{
			case TextureCompression::None: return srgb ? ImageFormat::SRGB_ALPHA : ImageFormat::RGBA8;
			case TextureCompression::DXT1: return srgb ? ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1 : ImageFormat::COMPRESSED_RGBA_S3TC_DXT1;
			case TextureCompression::DXT5: return srgb ? ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5 : ImageFormat::COMPRESSED_RGBA_S3TC_DXT5;
		}
	}
	else if(channels==3)
	{
		switch(compression)
		{
			case TextureCompression::None: return srgb ? ImageFormat::SRGB8 : ImageFormat::RGB8;
			case TextureCompression::DXT1: return srgb ? ImageFormat::COMPRESSED_SRGB_S3TC_DXT1 : ImageFormat::COMPRESSED_RGB_S3TC_DXT1;
			default:
			{
				DLOGE("texture") << "Unsupported compression option, defaulting to RGB8." << std::endl;
				return ImageFormat::RGB8;
			}
		}
	}
	else
	{
		DLOGE("texture") << "Only 3 or 4 color channels supported, but got: " << int(channels) << std::endl;
		DLOGI << "Defaulting to RGBA8." << std::endl;
		return ImageFormat::RGBA8;
	}
}

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

TextureGroup AssetManager::load_texture_group(const fs::path& filepath)
{
	fs::path fullpath = filesystem::get_asset_dir() / filepath;

	// Sanity check
	// If filepath is empty, return default invalid handle
	if(filepath.empty())
		return {};
	if(!fs::exists(fullpath))
	{
		DLOGE("texture") << "File does not exist." << std::endl;
		return {};
	}
	if(filepath.extension().string().compare(".tom"))
	{
		DLOGE("texture") << "Invalid input file." << std::endl;
		return {};
	}

	DLOGN("asset") << "[AssetManager] Creating new texture group:" << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	TextureGroup tg;

	DLOG("texture",1) << "Loading TOM file" << std::endl;
	tom::TOMDescriptor descriptor;
	descriptor.filepath = fullpath;
	tom::read_tom(descriptor);

	// Create and register all texture maps
	for(auto&& tmap: descriptor.texture_maps)
	{
		ImageFormat format = select_image_format(tmap.channels, tmap.compression, tmap.srgb);
		TextureHandle tex = Renderer::create_texture_2D(Texture2DDescriptor{descriptor.width,
									  					 				    descriptor.height,
									  					 				    3,
									  					 				    tmap.data,
									  					 				    format,
									  					 				    tmap.filter,
									  					 				    descriptor.address_UV,
									  					 					TF_MUST_FREE}); // Let the renderer free the resources once the texture is loaded
		tg.textures[tg.texture_count++] = tex;
	}

#ifdef W_DEBUG
	DLOGI << "Found " << WCC('v') << tg.texture_count << WCC(0) << " texture maps. TextureHandles: { ";
	for(size_t ii=0; ii<tg.texture_count; ++ii)
	{
		DLOG("texture",1) << WCC('v') << tg.textures[ii].index << " ";
	}
	DLOG("texture",1) << WCC(0) << "}" << std::endl;
#endif

	return tg;
}

TextureHandle AssetManager::load_image(const fs::path& filepath, Texture2DDescriptor& descriptor, bool engine_path)
{
	DLOGN("asset") << "[AssetManager] Loading HDR file:" << std::endl;
	DLOGI << WCC('p') << filepath << WCC(0) << std::endl;
	
	fs::path fullpath;
	if(engine_path)
		fullpath = filesystem::get_system_asset_dir() / filepath;
	else
		fullpath = filesystem::get_asset_dir() / filepath;

	 
	// Sanity check
	if(!fs::exists(fullpath))
	{
		DLOGW("asset") << "[AssetManager] File does not exist. Returning invalid handle." << std::endl;
		return TextureHandle();
	}

	hash_t hextension = H_(filepath.extension().string().c_str());

	switch(hextension)
	{
		case ".hdr"_h:
		{
			// Load HDR file
			img::HDRDescriptor hdrfile { fullpath };
			img::read_hdr(hdrfile);

			DLOGI << "Width:    " << WCC('v') << hdrfile.width << std::endl;
			DLOGI << "Height:   " << WCC('v') << hdrfile.height << std::endl;
			DLOGI << "Channels: " << WCC('v') << hdrfile.channels << std::endl;

			if(2*hdrfile.height != hdrfile.width)
			{
				DLOGW("asset") << "[AssetManager] HDR file must be in 2:1 format (width = 2 * height) for optimal results." << std::endl;
			}

			descriptor.width  = hdrfile.width;
			descriptor.height = hdrfile.height;
			descriptor.mips = 0;
			descriptor.data = hdrfile.data;
			descriptor.image_format = ImageFormat::RGB32F;
			descriptor.flags = TF_MUST_FREE; // Let the renderer free the resources once the texture is loaded

			// Create texture
			return Renderer::create_texture_2D(descriptor); 
			break;
		}
		case ".png"_h:
		{
			// Load PNG file
			img::PNGDescriptor pngfile { fullpath };
			// Force 4 channels
			pngfile.channels = 4;
			img::read_png(pngfile);

			DLOGI << "Width:    " << WCC('v') << pngfile.width << std::endl;
			DLOGI << "Height:   " << WCC('v') << pngfile.height << std::endl;
			DLOGI << "Channels: " << WCC('v') << pngfile.channels << std::endl;

			descriptor.width  = pngfile.width;
			descriptor.height = pngfile.height;
			descriptor.data = pngfile.data;
			descriptor.flags = TF_MUST_FREE; // Let the renderer free the resources once the texture is loaded

			// Create texture
			return Renderer::create_texture_2D(descriptor);
			break;
		}
		default:
		{
			DLOGW("asset") << "[AssetManager] Unknown file type: " << filepath.extension().string() << std::endl;
			DLOGI << "Returning invalid texture handle." << std::endl;
		}
	}

	return TextureHandle();
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

const Material& AssetManager::create_material(const std::string& name,
									  		  const TextureGroup& tg,
									  		  ShaderHandle shader,
									  		  UniformBufferHandle ubo,
									  		  uint32_t data_size,
									  		  bool is_public)
{
	hash_t archetype = H_(name.c_str());

	auto it = s_storage.materials_.find(archetype);
	if(it!=s_storage.materials_.end())
		return it->second;

	s_storage.materials_.insert({archetype, {archetype, tg, shader, ubo, data_size}});
	s_storage.material_descriptors_.insert({archetype, {is_public, name, ""}});
	Renderer3D::register_shader(shader);
	if(ubo.index != k_invalid_handle)
		Renderer::shader_attach_uniform_buffer(shader, ubo);
	return s_storage.materials_.at(archetype);
}

const Material& AssetManager::get_material(hash_t archetype)
{
	auto it = s_storage.materials_.find(archetype);
	W_ASSERT_FMT(it!=s_storage.materials_.end(), "Unknown material: %s", istr::resolve(archetype).c_str());
	return it->second;
}

const std::string& AssetManager::get_material_name(hash_t arch_name)
{
	auto it = s_storage.material_descriptors_.find(arch_name);
	static const std::string empty_str = "";
	if(it!=s_storage.material_descriptors_.end())
		return it->second.name;
	else
		return empty_str;
}

void AssetManager::visit_materials(MaterialVisitor visit)
{
	for(auto&& [archetype, material]: s_storage.materials_)
	{
		const auto& desc = s_storage.material_descriptors_.at(archetype);
		if(!desc.is_public)
			continue;
		if(visit(material, desc.name, desc.description))
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

void AssetManager::release(TextureGroup tg)
{
	DLOGN("asset") << "[AssetManager] Releasing texture group." << std::endl;

	for(auto&& tex_handle: tg.textures)
		if(tex_handle.is_valid())
			Renderer::destroy(tex_handle);
}

void AssetManager::release(hash_t archetype)
{
	auto it = s_storage.materials_.find(archetype);
	if(it==s_storage.materials_.end())
	{
		DLOGW("asset") << "[AssetManager] Cannot release unknown material: " << istr::resolve(archetype) << std::endl;
		return;
	}

	DLOGN("asset") << "[AssetManager] Releasing material:" << std::endl;
	DLOGI << WCC('n') << istr::resolve(archetype) << std::endl;
	s_storage.materials_.erase(it);
	s_storage.material_descriptors_.erase(archetype);
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

	s_storage.texture_atlases_.resize(k_max_atlases, nullptr);
	s_storage.font_atlases_.resize(k_max_font_atlases, nullptr);

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


} // namespace erwin