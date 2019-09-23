#include "render/texture_atlas.h"

#include <random>

namespace erwin
{


TextureAtlas::TextureAtlas()
{
	
}

TextureAtlas::~TextureAtlas()
{
	
}

void TextureAtlas::load(const fs::path& png_file, const fs::path& remapping_file)
{
	texture_ = Texture2D::create(png_file);

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

	        // Calculate UVs and save in remapping table
	        remapping_.insert(std::make_pair(H_(key.c_str()), glm::vec4{x/width, y/height, (x+w)/width, (y+h)/height}));
	    }
	}
}

void TextureAtlas::load(const fs::path& dxa_file)
{

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