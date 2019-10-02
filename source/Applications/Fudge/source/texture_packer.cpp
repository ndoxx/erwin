#include "texture_packer.h"
#include "dxt_compressor.h"

#include "core/tom_file.h"
#include "core/xml_file.h"
#include "render/texture_common.h"
#include "debug/logger.h"

#include "stb/stb_image.h"
#include <fstream>
#include <unordered_map>

using namespace erwin;

namespace fudge
{
namespace texmap
{

struct TexmapData
{
    std::string name;   // Name of texture map
    stbi_uc* data;      // Image data
    int width;          // Size of image
    int height;
    int channels;       // Number of color channels
};

struct TexmapSpec
{
	std::string name;
	TextureFilter filter = TextureFilter(MAG_NEAREST | MIN_NEAREST);
	uint32_t channels = 1;
	TextureCompression compression = TextureCompression::None;
	bool srgb = false;
};

static Compression s_blob_compression = Compression::Deflate;
static std::unordered_map<hash_t, TexmapSpec> s_texmap_specs;

void set_compression(Compression compression)
{
	s_blob_compression = compression;
}

bool configure(const fs::path& filepath)
{
	xml::XMLFile cfg(filepath);
	if(!cfg.read())
	{
		DLOGE("fudge") << "Failed to parse configuration file." << std::endl;
		return false;
	}

	auto* tmaps_node = cfg.root->first_node("TextureMaps");
	if(!tmaps_node)
	{
		DLOGE("fudge") << "Unable to find TextureMaps node." << std::endl;
		return false;
	}

    for(auto* tmap_node=tmaps_node->first_node("TextureMap");
        tmap_node;
        tmap_node=tmap_node->next_sibling("TextureMap"))
    {
    	TexmapSpec spec;
    	std::string compression_str("None");
        xml::parse_attribute(tmap_node, "name", spec.name);
        xml::parse_attribute(tmap_node, "channels", spec.channels);
        if(xml::parse_attribute(tmap_node, "compression", compression_str))
        {
        	switch(H_(compression_str.c_str()))
        	{
        		case "None"_h: spec.compression = TextureCompression::None; break;
        		case "DXT5"_h:
        		{
        			if(spec.channels != 4)
        			{
        				DLOGE("fudge") << "Need 4 color channels for DXT5 compression, got: " << spec.channels << std::endl;
        				return false;
        			}
        			spec.compression = TextureCompression::DXT5;
        			break;
        		}
        		default:
        		{
        			DLOGW("fudge") << "Unrecognized compression string: " << compression_str << std::endl;
        			DLOGI << "No compression will be set." << std::endl;
        			spec.compression = TextureCompression::None;
        			break;
        		}
        	}
        }

        DLOG("fudge",1) << "Texture map: " << WCC('n') << spec.name << std::endl;
        DLOGI << "channels:    " << spec.channels << std::endl;
        DLOGI << "compression: " << compression_str << std::endl;

        s_texmap_specs.insert(std::make_pair(H_(spec.name.c_str()), spec));
    }

    return true;
}

void make_tom(const fs::path& input_dir, const fs::path& output_dir)
{
    std::vector<TexmapData> texture_maps;

    uint32_t width = 0;
    uint32_t height = 0;

    // * Iterate over all files and load them
    for(auto& entry: fs::directory_iterator(input_dir))
    {
        if(entry.is_regular_file())
        {
            TexmapData tmap;
            tmap.name = entry.path().stem().string();

            // Make sure this texture map is properly described
            hash_t tmap_hname = H_(tmap.name.c_str());
            auto it = s_texmap_specs.find(tmap_hname);
            if(it == s_texmap_specs.end())
            {
            	DLOGW("fudge") << "Skipping unrecognized texture map: " << tmap.name << std::endl;
            	DLOGI << "Describe it in the XML config file." << std::endl;
            	continue;
            }
            const TexmapSpec& spec = it->second;

            // Load data
            tmap.data = stbi_load(entry.path().string().c_str(), &tmap.width, &tmap.height, &tmap.channels, spec.channels);
            if(!tmap.data)
            {
    			DLOGE("fudge") << "Error while loading image: " << entry.path().filename() << std::endl;
                continue;
            }

            // Check size is homogeneous across all maps
            if(width==0 && height==0)
            {
            	width = tmap.width;
            	height = tmap.height;
            }
            else
            {
            	if(tmap.width != width || tmap.height != height)
            	{
            		DLOGW("fudge") << "Skipping texture map with inconsistent size." << std::endl;
            		DLOGI << "Expected: " << width << "x" << height << std::endl;
            		DLOGI << "But got:  " << tmap.width << "x" << tmap.height << std::endl;
        			stbi_image_free(tmap.data);
            		continue;
            	}
            }

            texture_maps.push_back(tmap);
        }
    }

    // TODO: Compress textures (DXT...)

    // Write TOM file
	std::string out_file_name = input_dir.stem().string() + ".tom";
	tom::TOMDescriptor tom_desc
	{
		output_dir / out_file_name,
		(uint16_t)width,
		(uint16_t)height,
		((s_blob_compression==Compression::Deflate) ? tom::LosslessCompression::Deflate : tom::LosslessCompression::None),
		TextureWrap::REPEAT
	};

	for(auto&& tmap: texture_maps)
	{
		hash_t hname = H_(tmap.name.c_str());
        const TexmapSpec& spec = s_texmap_specs.at(hname);
        uint32_t size = width * height * spec.channels;
		tom::TextureMapDescriptor tm_desc
		{
			spec.filter,
			(uint8_t)spec.channels,
			spec.srgb,
			spec.compression,
			size,
			tmap.data,
			hname
		};

		tom_desc.texture_maps.push_back(tm_desc);
	}

	DLOG("fudge",1) << "Exporting: " << WCC('p') << out_file_name << std::endl;
	tom::write_tom(tom_desc);

    // Cleanup
    for(auto&& tmap: texture_maps)
        stbi_image_free(tmap.data);
}

} // namespace texmap
} // namespace fudge