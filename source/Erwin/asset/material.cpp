#include "asset/material.h"
#include "render/texture_common.h"
#include "render/main_renderer.h"

namespace erwin
{

void Material::load(const fs::path& filepath)
{
	// Check file type
	std::string extension = filepath.extension().string();
	if(!extension.compare(".tom"))
	{
		DLOG("texture",1) << "Loading TOM file" << std::endl;
		tom::TOMDescriptor descriptor;
		descriptor.filepath = filepath;
		tom::read_tom(descriptor);

		// Create and register all texture maps
		for(auto&& tmap: descriptor.texture_maps)
		{
			// TODO: handle sRGB hint
			ImageFormat format;
			switch(tmap.compression)
			{
				case TextureCompression::None: format = ImageFormat::SRGB_ALPHA; break;
				case TextureCompression::DXT1: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1; break;
				case TextureCompression::DXT5: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5; break;
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
}

void Material::release()
{
	for(auto&& handle: textures)
		if(handle.is_valid())
			MainRenderer::destroy(handle);
		
	// Resources allocated by the descriptor are located inside the filesystem's resource arena
	// This arena should be reset frequently, we don't need to care about freeing the resources here
}


} // namespace erwin