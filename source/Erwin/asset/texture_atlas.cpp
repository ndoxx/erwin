#include "asset/texture_atlas.h"
#include "render/main_renderer.h"
#include "core/z_wrapper.h"
#include "debug/logger.h"

namespace erwin
{

void TextureAtlas::load(const fs::path& filepath)
{
	// Check file type
	std::string extension = filepath.extension().string();
	if(!extension.compare(".cat"))
	{
		DLOG("texture",1) << "Loading CAT file" << std::endl;
		cat::CATDescriptor descriptor;
		descriptor.filepath = filepath;
		cat::read_cat(descriptor);

		width = descriptor.texture_width;
		height = descriptor.texture_height;
		float fwidth = width;
		float fheight = height;

		// Apply half-pixel correction if linear filtering is used
		/*glm::vec4 correction = (filter & MAG_LINEAR) ? glm::vec4(0.5f/fwidth, 0.5f/fheight, -0.5f/fwidth, -0.5f/fheight)
													 : glm::vec4(0.f);*/

		cat::traverse_remapping(descriptor, [&](const cat::CATAtlasRemapElement& remap)
		{
			glm::vec4 uvs((remap.x)/fwidth, (remap.y)/fheight, 
					      (remap.x+remap.w)/fwidth, (remap.y+remap.h)/fheight);
		    remapping.insert(std::make_pair(H_(remap.name), uvs/*+correction*/));
		});

		// Create texture
		ImageFormat format;
		switch(descriptor.texture_compression)
		{
			case TextureCompression::None: format = ImageFormat::SRGB_ALPHA; break;
			case TextureCompression::DXT1: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1; break;
			case TextureCompression::DXT5: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5; break;
		}
		uint8_t filter = MAG_NEAREST | MIN_NEAREST;
		// uint8_t filter = MAG_NEAREST | MIN_LINEAR_MIPMAP_NEAREST;
		// uint8_t filter = MAG_LINEAR | MIN_NEAREST_MIPMAP_NEAREST;

		texture = MainRenderer::create_texture_2D(Texture2DDescriptor{descriptor.texture_width,
									  					 			  descriptor.texture_height,
									  					 			  descriptor.texture_blob,
									  					 			  format,
									  					 			  filter});

		DLOG("texture",1) << "Found " << WCC('v') << remapping.size() << WCC(0) << " sub-textures in atlas." << std::endl;
		DLOG("texture",1) << "TextureHandle: " << WCC('v') << texture.index << std::endl;
	}
}

void TextureAtlas::release()
{
	MainRenderer::destroy(texture);
	
	// Resources allocated by the descriptor are located inside the filesystem's resource arena
	// This arena should be reset frequently, we don't need to care about freeing the resources here
}


} // namespace erwin