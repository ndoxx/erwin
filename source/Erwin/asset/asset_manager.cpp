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
#include "core/intern_string.h"

#ifdef W_USE_EASTL
	#include "EASTL/vector.h"
	#include "EASTL/map.h"
	#include "core/eastl_new.h" // new overloads needed by EASTL (linker error otherwise)
#else
	#include <vector>
	#include <map>
#endif

namespace erwin
{

#define FOR_ALL_HANDLES                   \
		DO_ACTION( TextureAtlasHandle )   \
		DO_ACTION( FontAtlasHandle )


static struct
{
#ifdef W_USE_EASTL
	eastl::vector<TextureAtlas*> texture_atlases_;
	eastl::vector<FontAtlas*> font_atlases_;

	eastl::map<hash_t, TextureHandle> special_textures_cache_;
	eastl::map<hash_t, TextureAtlasHandle> atlas_cache_;
	eastl::map<hash_t, FontAtlasHandle> font_cache_;
	eastl::map<hash_t, ShaderHandle> shader_cache_;
	eastl::map<uint64_t, UniformBufferHandle> ubo_cache_;

	eastl::map<hash_t, ComponentPBRMaterial> pbr_materials_;
#else
	std::vector<TextureAtlas*> texture_atlases_;
	std::vector<FontAtlas*> font_atlases_;

	std::map<hash_t, TextureHandle> special_textures_cache_;
	std::map<hash_t, TextureAtlasHandle> atlas_cache_;
	std::map<hash_t, FontAtlasHandle> font_cache_;
	std::map<hash_t, ShaderHandle> shader_cache_;
	std::map<uint64_t, UniformBufferHandle> ubo_cache_;

	std::map<hash_t, ComponentPBRMaterial> pbr_materials_;
#endif

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

constexpr uint8_t k_dark  = 153;
constexpr uint8_t k_light = 255;

[[maybe_unused]] static void dashed_texture(uint8_t* buffer, uint32_t size_px)
{
	for(uint32_t yy=0; yy<size_px; ++yy)
	{
		for(uint32_t xx=0; xx<size_px; ++xx)
		{
			uint32_t index = 3*(yy*size_px+xx);
			bool step = (xx+yy)%16 < 8;
			buffer[index+0] = step ? k_light : k_dark;
			buffer[index+1] = step ? k_light : k_dark;
			buffer[index+2] = step ? k_light : k_dark;
		}
	}
}

[[maybe_unused]] static void grid_texture(uint8_t* buffer, uint32_t size_px)
{
	for(uint32_t yy=0; yy<size_px; ++yy)
	{
		for(uint32_t xx=0; xx<size_px; ++xx)
		{
			uint32_t index = 3*(yy*size_px+xx);
			bool step = (xx%16 < 8) && (yy%16 < 8);
			buffer[index+0] = step ? k_light : k_dark;
			buffer[index+1] = step ? k_light : k_dark;
			buffer[index+2] = step ? k_light : k_dark;
		}
	}
}

[[maybe_unused]] static void checkerboard_texture(uint8_t* buffer, uint32_t size_px)
{
	for(uint32_t yy=0; yy<size_px; ++yy)
	{
		for(uint32_t xx=0; xx<size_px; ++xx)
		{
			uint32_t index = 3*(yy*size_px+xx);
			bool step = ((xx%16 < 8) && (yy%16 < 8)) || ((xx%16 > 8) && (yy%16 > 8));
			buffer[index+0] = step ? k_light : k_dark;
			buffer[index+1] = step ? k_light : k_dark;
			buffer[index+2] = step ? k_light : k_dark;
		}
	}
}

[[maybe_unused]] static void colored_texture(uint8_t* buffer, uint32_t size_px, uint8_t r, uint8_t g, uint8_t b)
{
	for(uint32_t yy=0; yy<size_px; ++yy)
	{
		for(uint32_t xx=0; xx<size_px; ++xx)
		{
			uint32_t index = 3*(yy*size_px+xx);
			buffer[index+0] = r;
			buffer[index+1] = g;
			buffer[index+2] = b;
		}
	}
}

TextureHandle AssetManager::create_debug_texture(hash_t type, uint32_t size_px)
{
	W_PROFILE_FUNCTION()

	// First, check cache
	hash_t hname = HCOMBINE_(type, hash_t(size_px));
	auto it = s_storage.special_textures_cache_.find(hname);
	if(it != s_storage.special_textures_cache_.end())
		return it->second;

	// Create checkerboard pattern
	uint8_t* buffer = new uint8_t[size_px*size_px*3];
	switch(type)
	{
		case "dashed"_h:       dashed_texture(buffer, size_px); break;
		case "grid"_h:         grid_texture(buffer, size_px); break;
		case "checkerboard"_h: checkerboard_texture(buffer, size_px); break;
		case "white"_h:        colored_texture(buffer, size_px, 255, 255, 255); break;
		case "red"_h:          colored_texture(buffer, size_px, 255, 0, 0); break;
		default:               checkerboard_texture(buffer, size_px); hname = "checkerboard"_h; break;
	}

	Texture2DDescriptor descriptor;
	descriptor.width  = size_px;
	descriptor.height = size_px;
	descriptor.mips = 0;
	descriptor.data = buffer;
    descriptor.image_format = ImageFormat::RGB8;
	descriptor.flags = TF_MUST_FREE; // Let the renderer free the resources once the texture is loaded

	// Create texture
	TextureHandle tex = Renderer::create_texture_2D(descriptor);
	s_storage.special_textures_cache_.insert({hname, tex});
	return tex;
}


// TODO: Cache lookup before creating any resource
TextureAtlasHandle AssetManager::load_texture_atlas(const fs::path& filepath)
{
	W_PROFILE_FUNCTION()

	hash_t hname = H_(filepath.string().c_str());
	auto it = s_storage.atlas_cache_.find(hname);
	if(it!=s_storage.atlas_cache_.end())
		return it->second;

	TextureAtlasHandle handle = TextureAtlasHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new texture atlas:" << std::endl;
	DLOG("asset",1) << "TextureAtlasHandle: " << WCC('v') << handle.index << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	TextureAtlas* atlas = W_NEW(TextureAtlas, s_storage.texture_atlas_pool_);
	atlas->load(filepath);

	// Register atlas
	Renderer2D::create_batch(atlas->texture); // TODO: this should be conditional
	s_storage.texture_atlases_[handle.index] = atlas;
	s_storage.atlas_cache_.insert({hname, handle});

	return handle;
}

FontAtlasHandle AssetManager::load_font_atlas(const fs::path& filepath)
{
	W_PROFILE_FUNCTION()

	hash_t hname = H_(filepath.string().c_str());
	auto it = s_storage.font_cache_.find(hname);
	if(it!=s_storage.font_cache_.end())
		return it->second;

	FontAtlasHandle handle = FontAtlasHandle::acquire();
	DLOGN("asset") << "[AssetManager] Creating new font atlas:" << std::endl;
	DLOG("asset",1) << "FontAtlasHandle: " << WCC('v') << handle.index << std::endl;
	DLOG("asset",1) << WCC('p') << filepath << WCC(0) << std::endl;

	FontAtlas* atlas = W_NEW(FontAtlas, s_storage.font_atlas_pool_);
	atlas->load(filepath);

	// Register atlas
	s_storage.font_atlases_[handle.index] = atlas;
	s_storage.font_cache_.insert({hname, handle});

	return handle;
}

TextureHandle AssetManager::load_image(const fs::path& filepath, Texture2DDescriptor& descriptor)
{
	W_PROFILE_FUNCTION()

	DLOGN("asset") << "[AssetManager] Loading HDR file:" << std::endl;
	DLOGI << WCC('p') << filepath << WCC(0) << std::endl;
	 
	// Sanity check
	if(!fs::exists(filepath))
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
			img::HDRDescriptor hdrfile { filepath };
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
		}
		case ".png"_h:
		{
			// Load PNG file
			img::PNGDescriptor pngfile { filepath };
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
	W_PROFILE_FUNCTION()

	hash_t hname = H_(filepath.string().c_str());
	auto it = s_storage.shader_cache_.find(hname);
	if(it!=s_storage.shader_cache_.end())
		return it->second;

	DLOGN("asset") << "[AssetManager] Creating new shader:" << std::endl;

	// First, check if shader file exists in system assets
	fs::path fullpath;
	if(fs::exists(wfs::get_system_asset_dir() / filepath))
		fullpath = wfs::get_system_asset_dir() / filepath;
	else
		fullpath = wfs::get_asset_dir() / filepath;

	std::string shader_name = name.empty() ? filepath.stem().string() : name;
	ShaderHandle handle = Renderer::create_shader(fullpath, shader_name);
	DLOG("asset",1) << "ShaderHandle: " << WCC('v') << handle.index << std::endl;
	s_storage.shader_cache_.insert({hname, handle});

	return handle;
}

UniformBufferHandle AssetManager::create_material_data_buffer(uint64_t component_id, uint32_t size)
{
	W_PROFILE_FUNCTION()

	auto it = s_storage.ubo_cache_.find(component_id);
	if(it!=s_storage.ubo_cache_.end())
		return it->second;

	auto handle = Renderer::create_uniform_buffer("material_data", nullptr, size, UsagePattern::Dynamic);
	s_storage.ubo_cache_.insert({component_id, handle});
	return handle;
}

const ComponentPBRMaterial& AssetManager::load_PBR_material(const fs::path& tom_path)
{
	W_PROFILE_FUNCTION()

	// * Sanity check
	W_ASSERT(fs::exists(tom_path), "[AssetManager] File does not exist.");
	W_ASSERT(!tom_path.extension().string().compare(".tom"), "[AssetManager] Invalid input file.");

	// * Check cache first
	hash_t hname = H_(tom_path.string().c_str());
	auto it = s_storage.pbr_materials_.find(hname);
	if(it!=s_storage.pbr_materials_.end())
	{
		DLOG("asset",1) << "[AssetManager] Loading PBR material " << WCC('i') << "from cache" << WCC(0) << ":" << std::endl;
		DLOG("asset",1) << WCC('p') << tom_path << WCC(0) << std::endl;
		return it->second;
	}

	DLOGN("asset") << "[AssetManager] Loading PBR material:" << std::endl;
	DLOG("asset",1) << WCC('p') << tom_path << WCC(0) << std::endl;

	TextureGroup tg;

	tom::TOMDescriptor descriptor;
	descriptor.filepath = tom_path;
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


	W_ASSERT(descriptor.material_type == tom::MaterialType::PBR, "[AssetManager] Material is not PBR.");
	W_ASSERT(descriptor.material_data_size == sizeof(ComponentPBRMaterial::MaterialData), "[AssetManager] Invalid material data size.");
	ShaderHandle shader     = load_shader("shaders/deferred_PBR.glsl");
	UniformBufferHandle ubo = create_material_data_buffer<ComponentPBRMaterial>();
	
	std::string name = tom_path.stem().string();

	Material mat = {H_(name.c_str()), tg, shader, ubo, sizeof(ComponentPBRMaterial::MaterialData)};
	Renderer3D::register_shader(shader);
	Renderer::shader_attach_uniform_buffer(shader, ubo);

	ComponentPBRMaterial pbr_mat;
	pbr_mat.set_material(mat);
	memcpy(&pbr_mat.material_data, descriptor.material_data, descriptor.material_data_size);
	delete[] descriptor.material_data;

	s_storage.pbr_materials_.emplace(hname, std::move(pbr_mat));

	return s_storage.pbr_materials_.at(hname);
}


#ifdef W_USE_EASTL
	template <typename KeyT, typename HandleT>
	static void erase_by_value(eastl::map<KeyT, HandleT>& cache, HandleT handle)
	{
		auto it = cache.begin();
		for(; it!=cache.end(); ++it)
	    	if(it->second == handle)
	    		break;
		cache.erase(it);
	}
#else
	template <typename KeyT, typename HandleT>
	static void erase_by_value(std::map<KeyT, HandleT>& cache, HandleT handle)
	{
		auto it = cache.begin();
		for(; it!=cache.end(); ++it)
	    	if(it->second == handle)
	    		break;
		cache.erase(it);
	}
#endif

void AssetManager::release(TextureAtlasHandle handle)
{
	W_PROFILE_FUNCTION()

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
	W_PROFILE_FUNCTION()

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

void AssetManager::release(ShaderHandle handle)
{
	W_PROFILE_FUNCTION()

	W_ASSERT_FMT(handle.is_valid(), "ShaderHandle of index %hu is invalid.", handle.index);
	DLOGN("asset") << "[AssetManager] Releasing shader:" << std::endl;

	erase_by_value(s_storage.shader_cache_, handle);
	Renderer::destroy(handle);

	DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
}

void AssetManager::release(UniformBufferHandle handle)
{
	W_PROFILE_FUNCTION()

	erase_by_value(s_storage.ubo_cache_, handle);
	Renderer::destroy(handle);
}

// ---------------- PRIVATE API ----------------

void AssetManager::init(memory::HeapArea& area)
{
	W_PROFILE_FUNCTION()

	s_storage.handle_arena_.init(area, k_asset_handle_alloc_size, "AssetHandles");
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
	W_PROFILE_FUNCTION()
	
	for(TextureAtlas* atlas: s_storage.texture_atlases_)
		if(atlas)
			W_DELETE(atlas, s_storage.texture_atlas_pool_);

	for(FontAtlas* atlas: s_storage.font_atlases_)
		if(atlas)
			W_DELETE(atlas, s_storage.font_atlas_pool_);

	for(auto&& [hname,tex]: s_storage.special_textures_cache_)
		Renderer::destroy(tex);

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
