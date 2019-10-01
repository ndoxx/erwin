#include "texture_packer.h"
#include "dxt_compressor.h"

#include "core/tom_file.h"
#include "debug/logger.h"

#include "stb/stb_image.h"
#include <fstream>

using namespace erwin;

namespace fudge
{
namespace texmap
{

// For image texture atlas
struct TexmapData
{
    std::string name;   // Name of texture map
    stbi_uc* data;      // Image data
    int width;          // Size of image
    int height;
    int channels;       // Number of color channels
};

static Compression s_blob_compression = Compression::Deflate;

void set_compression(Compression compression)
{
	s_blob_compression = compression;
}

void make_tom(const fs::path& input_dir, const fs::path& output_dir)
{
    std::vector<TexmapData> texture_maps;

    // * Iterate over all files and load them
    for(auto& entry: fs::directory_iterator(input_dir))
    {
        if(entry.is_regular_file())
        {
            TexmapData tmap;
            tmap.name = entry.path().stem().string();
            tmap.data = stbi_load(entry.path().string().c_str(), &tmap.width, &tmap.height, &tmap.channels, 0);
            if(!tmap.data)
            {
    			DLOGE("fudge") << "Error while loading image: " << entry.path().filename() << std::endl;
                continue;
            }

            texture_maps.push_back(tmap);
        }
    }

    // Cleanup
    for(auto&& tmap: texture_maps)
        stbi_image_free(tmap.data);
}

} // namespace texmap
} // namespace fudge