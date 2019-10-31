#include "render/texture_atlas.h"
#include "filesystem/cat_file.h"
#include "core/z_wrapper.h"
#include "debug/logger.h"

#include <random>
#include <iostream>

namespace erwin
{


TextureAtlas::TextureAtlas()
{
	
}

TextureAtlas::~TextureAtlas()
{
	
}

void TextureAtlas::load(const fs::path& filepath)
{
	DLOGN("texture") << "Loading texture atlas: " << std::endl;

	// Check file type
	std::string extension = filepath.extension().string();
	if(!extension.compare(".png"))
	{
		// Path to remapping file
		std::string stem = filepath.stem().string();
		fs::path remapping_file = filepath.parent_path() / (stem + ".txt");

		if(!fs::exists(filesystem::get_asset_dir() / remapping_file))
		{
			DLOGW("texture") << "Missing remapping file!" << std::endl;
			return;
		}

		DLOGI << "png:   " << WCC('p') << filepath << WCC(0) << std::endl;
		DLOGI << "remap: " << WCC('p') << remapping_file << WCC(0) << std::endl;

		texture_ = Texture2D::create(filepath);

		float width = texture_->get_width();
		float height = texture_->get_height();

		std::ifstream ifs(filesystem::get_asset_dir() / remapping_file);
		std::string line;

		while(std::getline(ifs, line))
		{
		    if(line[0] != '#')
		    {
		    	// Get sub-texture name, position and dimensions
		        std::istringstream iss(line);
		        std::string key;
		        int x, y, w, h;
		        iss >> key >> x >> y >> w >> h;

		        // Calculate UVs for bottom left and top right corners
		        // Also apply half-pixel correction to address the texel centers 
		        // rather than the edges, and avoid bleeding
		        glm::vec4 uvs((x+0.5f)/width, (y+0.5f)/height, (x-0.5f+w)/width, (y-0.5f+h)/height);
		        // Save uvs in remapping table
		        remapping_.insert(std::make_pair(H_(key.c_str()), uvs));
		    }
		}
	}
	else if(!extension.compare(".cat"))
	{
		DLOGI << "CAT: " << WCC('p') << filepath << WCC(0) << std::endl;

		cat::CATDescriptor desc;
		desc.filepath = filesystem::get_asset_dir() / filepath;

		cat::read_cat(desc);

		ImageFormat format;
		switch(desc.texture_compression)
		{
			case TextureCompression::None: format = ImageFormat::SRGB_ALPHA; break;
			case TextureCompression::DXT1: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1; break;
			case TextureCompression::DXT5: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5; break;
		}
		uint8_t filter = MAG_NEAREST | MIN_LINEAR_MIPMAP_NEAREST;
		// uint8_t filter = MAG_LINEAR | MIN_NEAREST_MIPMAP_NEAREST;
		texture_ = Texture2D::create(Texture2DDescriptor{desc.texture_width,
									  					 desc.texture_height,
									  					 desc.texture_blob,
									  					 format,
									  					 filter});

		float width = texture_->get_width();
		float height = texture_->get_height();

		// Apply half-pixel correction if linear filtering is used
		glm::vec4 correction = (filter & MAG_LINEAR) ? glm::vec4(0.5f/width, 0.5f/height, -0.5f/width, -0.5f/height)
													 : glm::vec4(0.f);

		cat::traverse_remapping(desc, [&](const cat::CATAtlasRemapElement& remap)
		{
			glm::vec4 uvs((remap.x)/width, (remap.y)/height, 
					      (remap.x+remap.w)/width, (remap.y+remap.h)/height);
		    remapping_.insert(std::make_pair(H_(remap.name), uvs+correction));
		});

		desc.release();
	}

	DLOG("texture",1) << "Found " << remapping_.size() << " sub-textures in atlas." << std::endl;
}

static int rand_between(int low, int high)
{
    static std::random_device            seeder;                    // used to seed with system entropy
    static std::mt19937                  generator{seeder()};       // set up (and seed) the PRNG
    std::uniform_int_distribution<int>   distribution{low, high};   // set up distribution for requested range
    return distribution(generator);                                 // generate and return result
}

const glm::vec4& TextureAtlas::get_random_uv() const
{
	auto random_it = std::next(std::begin(remapping_), rand_between(0, remapping_.size()-1));
	return random_it->second;
}

} // namespace erwin