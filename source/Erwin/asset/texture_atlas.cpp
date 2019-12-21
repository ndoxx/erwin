#include "asset/texture_atlas.h"
#include "core/z_wrapper.h"
#include "debug/logger.h"

namespace erwin
{

void TextureAtlas::load(const fs::path& filepath)
{
	DLOGN("texture") << "Loading texture atlas: " << std::endl;

	// Check file type
	std::string extension = filepath.extension().string();
	if(!extension.compare(".cat"))
	{
		DLOGI << "CAT: " << WCC('p') << filepath << WCC(0) << std::endl;

		descriptor.filepath = filepath;
		cat::read_cat(descriptor);

		float width = get_width();
		float height = get_height();

		// Apply half-pixel correction if linear filtering is used
		/*glm::vec4 correction = (filter & MAG_LINEAR) ? glm::vec4(0.5f/width, 0.5f/height, -0.5f/width, -0.5f/height)
													 : glm::vec4(0.f);*/

		cat::traverse_remapping(descriptor, [&](const cat::CATAtlasRemapElement& remap)
		{
			glm::vec4 uvs((remap.x)/width, (remap.y)/height, 
					      (remap.x+remap.w)/width, (remap.y+remap.h)/height);
		    remapping.insert(std::make_pair(H_(remap.name), uvs/*+correction*/));
		});
	}

	DLOG("texture",1) << "Found " << remapping.size() << " sub-textures in atlas." << std::endl;
}

} // namespace erwin