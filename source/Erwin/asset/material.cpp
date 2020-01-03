#include "asset/material.h"
#include "render/texture_common.h"
#include "render/main_renderer.h"
#include "filesystem/tom_file.h"

namespace erwin
{

void TextureGroup::load(const fs::path& filepath, const MaterialLayout& layout)
{
	// Check file type
	std::string extension = filepath.extension().string();
	if(!extension.compare(".tom"))
	{
		DLOG("texture",1) << "Loading TOM file" << std::endl;
		tom::TOMDescriptor descriptor;
		descriptor.filepath = filepath;
		tom::read_tom(descriptor);

		// Check layout compatibility
		bool valid_layout = (layout.texture_count == descriptor.texture_maps.size());
		uint32_t slot = 0;
		for(auto&& tmap: descriptor.texture_maps)
			valid_layout &= (layout.texture_slots[slot++] == tmap.name);

		if(!valid_layout)
		{
			DLOGE("texture") << "Material layout is incompatible with this TOM file." << std::endl;
		}

		// Create and register all texture maps
		for(auto&& tmap: descriptor.texture_maps)
		{
			ImageFormat format;
			switch(tmap.compression)
			{
				case TextureCompression::None: format = tmap.srgb ? ImageFormat::SRGB_ALPHA : ImageFormat::RGBA8; break;
				case TextureCompression::DXT1: format = tmap.srgb ? ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1 : ImageFormat::COMPRESSED_RGBA_S3TC_DXT1; break;
				case TextureCompression::DXT5: format = tmap.srgb ? ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5 : ImageFormat::COMPRESSED_RGBA_S3TC_DXT5; break;
			}

			TextureHandle tex = MainRenderer::create_texture_2D(Texture2DDescriptor{descriptor.width,
										  					 				    	descriptor.height,
										  					 				    	tmap.data,
										  					 				    	format,
										  					 				    	tmap.filter,
										  					 				    	descriptor.address_UV});

			// TODO: choose slot according to texture map name
			textures[texture_count] = tex;
			++texture_count;
		}

#ifdef W_DEBUG
		DLOGI << "Found " << WCC('v') << texture_count << WCC(0) << " texture maps. TextureHandles: { ";
		for(int ii=0; ii<texture_count; ++ii)
		{
			DLOG("texture",1) << WCC('v') << textures[ii].index << " ";
		}
		DLOG("texture",1) << WCC(0) << "}" << std::endl;
#endif
	}
	else
	{
		DLOGE("texture") << "Invalid input file." << std::endl;
	}
}

void TextureGroup::release()
{
	for(auto&& handle: textures)
		if(handle.is_valid())
			MainRenderer::destroy(handle);

	// Resources allocated by the descriptor are located inside the filesystem's resource arena
	// This arena should be reset frequently, we don't need to care about freeing the resources here
}


} // namespace erwin