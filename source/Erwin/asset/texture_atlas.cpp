#include "asset/texture_atlas.h"
#include "render/renderer.h"
#include "core/z_wrapper.h"
#include "debug/logger.h"

namespace erwin
{

// BUG#5: CAT texture atlas files with no texture compression do not work
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

		// Make sure this is a texture atlas
		W_ASSERT(descriptor.remapping_type == cat::RemappingType::TextureAtlas, "Invalid remapping type for a texture atlas.");

		width = descriptor.texture_width;
		height = descriptor.texture_height;
		float fwidth = float(width);
		float fheight = float(height);

		// Apply half-pixel correction if linear filtering is used
		/*glm::vec4 correction = (filter & MAG_LINEAR) ? glm::vec4(0.5f/fwidth, 0.5f/fheight, -0.5f/fwidth, -0.5f/fheight)
													 : glm::vec4(0.f);*/

		cat::traverse_texture_remapping(descriptor, [&](const cat::CATAtlasRemapElement& remap)
		{
			glm::vec4 uvs((remap.x)/fwidth, (remap.y)/fheight, 
					      (remap.x+remap.w)/fwidth, (remap.y+remap.h)/fheight);
		    remapping.insert(eastl::make_pair(H_(remap.name), uvs/*+correction*/));
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

		texture = Renderer::create_texture_2D(Texture2DDescriptor{descriptor.texture_width,
									  					 			  descriptor.texture_height,
									  					 			  descriptor.texture_blob,
									  					 			  format,
									  					 			  filter});

		DLOG("texture",1) << "Found " << WCC('v') << remapping.size() << WCC(0) << " sub-textures in atlas." << std::endl;
		DLOG("texture",1) << "TextureHandle: " << WCC('v') << texture.index << std::endl;
	}
	else
	{
		DLOGE("texture") << "Invalid input file." << std::endl;
	}
}

void TextureAtlas::release()
{
	Renderer::destroy(texture);

	// Resources allocated by the descriptor are located inside the filesystem's resource arena
	// This arena should be reset frequently, we don't need to care about freeing the resources here
}



void FontAtlas::load(const fs::path& filepath)
{
	// Check file type
	std::string extension = filepath.extension().string();
	if(!extension.compare(".cat"))
	{
		DLOG("texture",1) << "Loading CAT file" << std::endl;
		cat::CATDescriptor descriptor;
		descriptor.filepath = filepath;
		cat::read_cat(descriptor);

		// Make sure this is a texture atlas
		W_ASSERT(descriptor.remapping_type == cat::RemappingType::FontAtlas, "Invalid remapping type for a font atlas.");

		width = descriptor.texture_width;
		height = descriptor.texture_height;
		float fwidth = float(width);
		float fheight = float(height);

		// Get maximal index and resize remapping table accordingly
		/*uint64_t max_index = 0;
		cat::traverse_font_remapping(descriptor, [&](const cat::CATFontRemapElement& remap)
		{
			if(remap.index > max_index)
				max_index = remap.index;
		});
		remapping.resize(max_index);*/

		// Half-pixel correction
		glm::vec4 correction = glm::vec4(0.5f/fwidth, 0.5f/fheight, -0.5f/fwidth, -0.5f/fheight);

		cat::traverse_font_remapping(descriptor, [&](const cat::CATFontRemapElement& remap)
		{
			glm::vec4 uvs((remap.x)/fwidth, (remap.y)/fheight, (remap.x+remap.w)/fwidth, (remap.y+remap.h)/fheight);
		    remapping[remap.index] = RemappingElement
		    {
		    	uvs+correction, 
		    	remap.w, 
		    	remap.h,
		    	remap.bearing_x,
		    	int16_t(remap.bearing_y-std::max(0,remap.h-remap.bearing_y)),
		    	remap.advance
		    };
		});

		// Create texture
		uint8_t filter = MAG_NEAREST | MIN_NEAREST;
		// uint8_t filter = MAG_NEAREST | MIN_LINEAR_MIPMAP_NEAREST;
		// uint8_t filter = MAG_LINEAR | MIN_NEAREST_MIPMAP_NEAREST;

		texture = Renderer::create_texture_2D(Texture2DDescriptor{descriptor.texture_width,
									  					 			  descriptor.texture_height,
									  					 			  descriptor.texture_blob,
									  					 			  ImageFormat::RGBA8,
									  					 			  // ImageFormat::R8,
									  					 			  filter});

		DLOG("texture",1) << "Found " << WCC('v') << remapping.size() << WCC(0) << " characters in atlas." << std::endl;
		DLOG("texture",1) << "TextureHandle: " << WCC('v') << texture.index << std::endl;
	}
	else
	{
		DLOGE("texture") << "Invalid input file." << std::endl;
	}
}

void FontAtlas::release()
{
	Renderer::destroy(texture);

	// Resources allocated by the descriptor are located inside the filesystem's resource arena
	// This arena should be reset frequently, we don't need to care about freeing the resources here
}

} // namespace erwin