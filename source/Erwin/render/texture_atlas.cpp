#include "render/texture_atlas.h"
#include "core/cat_file.h"
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

		        // Calculate UVs for bottom left and top right and save in remapping table
		        remapping_.insert(std::make_pair(H_(key.c_str()), glm::vec4{x/width, y/height, (x+w)/width, (y+h)/height}));
		    }
		}
	}
	else if(!extension.compare(".cat"))
	{
		DLOGI << "CAT: " << WCC('p') << filepath << WCC(0) << std::endl;

		CATDescriptor desc;
		desc.filepath = filesystem::get_asset_dir() / filepath;

		read_cat(desc);

		ImageFormat format;
		switch(desc.texture_compression)
		{
			case TextureCompression::None: format = ImageFormat::SRGB_ALPHA; break;
			case TextureCompression::DXT1: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1; break;
			case TextureCompression::DXT5: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5; break;
		}
		texture_ = Texture2D::create(Texture2DDescriptor{desc.texture_width,
									  					 desc.texture_height,
									  					 desc.texture_blob,
									  					 format,
									  					 MAG_NEAREST | MIN_LINEAR_MIPMAP_NEAREST});

		float width = texture_->get_width();
		float height = texture_->get_height();

		traverse_remapping(desc, [&](const CATAtlasRemapElement& remap)
		{
		    remapping_.insert(std::make_pair(H_(remap.name), glm::vec4{remap.x/width, remap.y/height, (remap.x+remap.w)/width, (remap.y+remap.h)/height}));
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