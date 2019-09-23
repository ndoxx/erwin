#include "render/texture_atlas.h"

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

	std::ifstream ifs(remapping_file);
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



} // namespace erwin