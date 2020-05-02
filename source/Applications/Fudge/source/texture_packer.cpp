#include "texture_packer.h"
#include "dxt_compressor.h"

#include "filesystem/tom_file.h"
#include "filesystem/xml_file.h"
#include "render/texture_common.h"
#include "debug/logger.h"

#include "stb/stb_image.h"
#include <fstream>
#include <map>

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
	TextureCompression compression;
};

struct TexmapSpec
{
	std::string name;
	TextureFilter filter = TextureFilter(MAG_NEAREST | MIN_NEAREST);
	TextureCompression compression = TextureCompression::None;
	uint32_t channels = 1;
	bool srgb = false;
};

struct GroupSpec
{
	TexmapSpec texmap_spec;

	std::vector<hash_t> sub_maps;
	std::vector<std::pair<hash_t, uint32_t>> layout;

	bool qualify(const std::map<hash_t, TexmapData>& tmaps)
	{
		bool ret = true;
		for(hash_t name: sub_maps)
			ret &= (tmaps.find(name) != tmaps.end());
		return ret;
	}
};

struct LayoutSpec
{
	std::vector<hash_t> slots;
	std::string name;
};

static BlobCompression s_blob_compression = BlobCompression::Deflate;
static std::map<hash_t, TexmapSpec> s_texmap_specs;
static std::vector<std::pair<hash_t, GroupSpec>> s_group_specs;
static std::vector<LayoutSpec> s_layout_specs;
static bool s_allow_grouping = false;

static TextureFilter parse_filter(const std::string& min_filter_str, const std::string& mag_filter_str)
{
	hash_t hmin_filter = H_(min_filter_str.c_str());
	hash_t hmag_filter = H_(mag_filter_str.c_str());

	uint8_t min_filter = 0;
	uint8_t mag_filter = 0;

	switch(hmag_filter)
	{
		case "NEAREST"_h: mag_filter = TextureFilter::MAG_NEAREST; break;
    	case "LINEAR"_h:  mag_filter = TextureFilter::MAG_LINEAR; break;
    	default:
    	{
    		DLOGW("fudge") << "Unrecognized magnification filter: " << mag_filter_str << std::endl;
    		DLOGI << "Defaulting to: " << WCC('d') << "NEAREST" << std::endl;
    		mag_filter = TextureFilter::MAG_NEAREST;
    	}
    }

	switch(hmin_filter)
	{
	    case "NEAREST"_h:                min_filter = TextureFilter::MIN_NEAREST; break;
	    case "LINEAR"_h:                 min_filter = TextureFilter::MIN_LINEAR; break;
	    case "NEAREST_MIPMAP_NEAREST"_h: min_filter = TextureFilter::MIN_NEAREST_MIPMAP_NEAREST; break;
	    case "LINEAR_MIPMAP_NEAREST"_h:  min_filter = TextureFilter::MIN_LINEAR_MIPMAP_NEAREST; break;
	    case "NEAREST_MIPMAP_LINEAR"_h:  min_filter = TextureFilter::MIN_NEAREST_MIPMAP_LINEAR; break;
	    case "LINEAR_MIPMAP_LINEAR"_h:   min_filter = TextureFilter::MIN_LINEAR_MIPMAP_LINEAR; break;
    	default:
    	{
    		DLOGW("fudge") << "Unrecognized minification filter: " << min_filter_str << std::endl;
    		DLOGI << "Defaulting to: " << WCC('d') << "NEAREST" << std::endl;
    		min_filter = TextureFilter::MIN_NEAREST;
    	}
	}

	return TextureFilter(min_filter | mag_filter);
}

static TextureCompression parse_compression(const std::string& compression_str, uint32_t channels)
{
	switch(H_(compression_str.c_str()))
	{
		case "None"_h: return TextureCompression::None;
		case "DXT5"_h:
		{
	        if(channels != 4)
			{
				DLOGE("fudge") << "Need 4 color channels for DXT5 compression, got: " << channels << std::endl;
				return TextureCompression::None;
			}
			return TextureCompression::DXT5;
		}
		default:
		{
			DLOGW("fudge") << "Unrecognized compression string: " << compression_str << std::endl;
			DLOGI << "No compression will be set." << std::endl;
			return TextureCompression::None;
		}
	}
}

static bool handle_groups(std::map<hash_t, TexmapData>& in_tex_maps, uint32_t width, uint32_t height)
{
	// For each registered group, ordered by priority,
	// if in_tex_maps contains all required maps for this group,
	// construct new texture map, copy data and override parameters,
	// then remove old maps.
	bool has_changed = false;
	for(auto&& [key, spec]: s_group_specs)
	{
		if(spec.qualify(in_tex_maps))
		{
        	DLOG("fudge",1) << "Qualifies for grouping under layout: " << WCC('n') << spec.texmap_spec.name << std::endl;
            TexmapData tmap
            {
            	spec.texmap_spec.name,
            	nullptr,
            	(uint16_t)width,
            	(uint16_t)height,
            	(int)spec.texmap_spec.channels,
            	spec.texmap_spec.compression
            };

            // Merge data according to spec layout
            tmap.data = new uint8_t[width*height*spec.texmap_spec.channels];
            for(int ii=0; ii<spec.texmap_spec.channels; ++ii)
	        {
	 			auto&& [hname, offset] = spec.layout[ii];
	 			uint8_t* sub_data = in_tex_maps.at(hname).data;
	 			// uint32_t sub_channels = in_tex_maps.at(hname).channels;

	 			// We force 4 color channels when loading image, so we assume here that sub_data
	 			// can be accessed the way it is. However, the output texture map can have only
	 			// 3 channels.

		        for(int yy=0; yy<tmap.height; ++yy)
		            for(int xx=0; xx<tmap.width; ++xx)
		                tmap.data[spec.texmap_spec.channels * (yy * width + xx) + ii] = sub_data[4 * (yy * width + xx) + offset];
		    }

		    // Remove old texture maps
		    for(hash_t hname: spec.sub_maps)
		    {
        		stbi_image_free(in_tex_maps.at(hname).data);
        		in_tex_maps.erase(hname);
		    }
		    // Add new texture map
		    in_tex_maps.insert(std::make_pair(H_(tmap.name.c_str()), tmap));
			has_changed = true;
		}
	}

	return has_changed;
}

static const LayoutSpec* find_layout(std::map<hash_t, TexmapData>& texture_maps)
{
	for(const auto& spec: s_layout_specs)
	{
		// For a layout to match, we need all of the layout's slots present in texture_maps,
		// nothing more and nothing less
		if(spec.slots.size() != texture_maps.size())
			continue;

		bool all_in = true;
		for(hash_t slot: spec.slots)
			all_in &= (texture_maps.find(slot)!=texture_maps.end());

		if(all_in)
			return &spec;
	}

	return nullptr;
}

bool configure(const fs::path& filepath)
{
	xml::XMLFile cfg(filepath);
	if(!cfg.read())
	{
		DLOGE("fudge") << "Failed to parse configuration file." << std::endl;
		return false;
	}

	// * Configure options
	auto* opt_node = cfg.root->first_node("Options");
	std::string blob_compression_str;
	if(xml::parse_node(opt_node, "BlobCompression", blob_compression_str))
	{
		if(!blob_compression_str.compare("DEFLATE"))
			s_blob_compression = BlobCompression::Deflate;
		else
			s_blob_compression = BlobCompression::None;
	}
	xml::parse_node(opt_node, "AllowGrouping", s_allow_grouping);

	// * Register texture maps
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
    	std::string compression_str;
        xml::parse_attribute(tmap_node, "name", spec.name);
        xml::parse_attribute(tmap_node, "channels", spec.channels);
        if(xml::parse_attribute(tmap_node, "compression", compression_str))
        	spec.compression = parse_compression(compression_str, spec.channels);

        std::string min_filter_str, mag_filter_str;
        if(xml::parse_attribute(tmap_node, "filter_min", min_filter_str) &&
           xml::parse_attribute(tmap_node, "filter_mag", mag_filter_str))
        	spec.filter = parse_filter(min_filter_str, mag_filter_str);

        xml::parse_attribute(tmap_node, "srgb", spec.srgb);

        DLOG("fudge",1) << "Texture map: " << WCC('n') << spec.name << std::endl;
        DLOGI << "channels:    " << spec.channels << std::endl;
        DLOGI << "compression: " << compression_str << std::endl;

        s_texmap_specs.insert(std::make_pair(H_(spec.name.c_str()), spec));
    }

    // Stop here if grouping is disabled
    if(!s_allow_grouping)
    	return true;

    // * Register groups
    for(auto* group_node=tmaps_node->first_node("Group");
        group_node;
        group_node=group_node->next_sibling("Group"))
    {
    	GroupSpec spec;
        xml::parse_attribute(group_node, "name", spec.texmap_spec.name);
        std::string compression_str;
        if(xml::parse_attribute(group_node, "compression", compression_str))
        	spec.texmap_spec.compression = parse_compression(compression_str, spec.texmap_spec.channels);
        std::string min_filter_str, mag_filter_str;
        if(xml::parse_attribute(group_node, "filter_min", min_filter_str) &&
           xml::parse_attribute(group_node, "filter_mag", mag_filter_str))
        	spec.texmap_spec.filter = parse_filter(min_filter_str, mag_filter_str);
        xml::parse_attribute(group_node, "srgb", spec.texmap_spec.srgb);

    	// For each texture sub-map
    	spec.texmap_spec.channels = 0;
	    for(auto* tmap_node=group_node->first_node("TextureMap");
	        tmap_node;
	        tmap_node=tmap_node->next_sibling("TextureMap"))
	    {
	    	std::string map_name;
        	xml::parse_attribute(tmap_node, "name", map_name);
	    	hash_t hmap_name = H_(map_name.c_str());

	    	const TexmapSpec& tmap_spec = s_texmap_specs.at(hmap_name);
	    	spec.texmap_spec.channels += tmap_spec.channels;

	    	// Handle layout
	    	spec.sub_maps.push_back(hmap_name);
	    	for(int ii=0; ii<tmap_spec.channels; ++ii)
	    		spec.layout.push_back(std::make_pair(hmap_name, ii));
	    }

	    // Sanity check
	    if(spec.texmap_spec.channels > 4)
	    {
	    	DLOGW("fudge") << "Skipping group with more than 4 channels." << std::endl;
	    	continue;
	    }

        DLOG("fudge",1) << "Group: " << WCC('n') << spec.texmap_spec.name << std::endl;
        DLOGI << "channels:    " << spec.texmap_spec.channels << std::endl;
        DLOGI << "compression: " << compression_str << std::endl;

	    s_group_specs.push_back(std::make_pair(H_(spec.texmap_spec.name.c_str()), spec));
	    // Also register internal texture map specs as a regular texture map spec.
	    s_texmap_specs.insert(std::make_pair(H_(spec.texmap_spec.name.c_str()), spec.texmap_spec));
    }

    // * Register layouts
    for(auto* layout_node=tmaps_node->first_node("Layout");
        layout_node;
        layout_node=layout_node->next_sibling("Layout"))
    {
    	LayoutSpec layout_spec;

    	// Get layout name
        xml::parse_attribute(layout_node, "name", layout_spec.name);

    	// Register each slot
	    for(auto* slot_node=layout_node->first_node("Slot");
	        slot_node;
	        slot_node=slot_node->next_sibling("Slot"))
	    {
	    	std::string texture;
        	xml::parse_attribute(slot_node, "texture", texture);
	    	layout_spec.slots.push_back(H_(texture.c_str()));
	    }
	    // Register layout
	    s_layout_specs.push_back(layout_spec);
    }

    return true;
}

void make_tom(const fs::path& input_dir, const fs::path& output_dir)
{
    DLOGN("fudge") << "Processing directory: " << WCC('p') << input_dir.stem() << std::endl;

    std::map<hash_t, TexmapData> texture_maps;

    uint32_t width = 0;
    uint32_t height = 0;

    // * Iterate over all files and load them
    for(auto&& entry: fs::directory_iterator(input_dir))
    {
        if(entry.is_regular_file())
        {
            DLOG("fudge",1) << "Reading: " << WCC('p') << entry.path().filename() << std::endl;

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
            tmap.data = stbi_load(entry.path().string().c_str(), &tmap.width, &tmap.height, &tmap.channels, 4);
            tmap.compression = spec.compression;
            if(!tmap.data)
            {
    			DLOGE("fudge") << "Error while loading image: " << entry.path().filename() << std::endl;
                continue;
            }

            // Check that number of color channels in input asset coincides with corresponding texture map specs 
            /*if(tmap.channels != spec.channels)
            {
            	DLOGW("fudge") << "Input image channels number mismatch: " << std::endl;
            	DLOGI << "Expected " << spec.channels << " but got " << tmap.channels << std::endl;
            }*/

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

            if(spec.srgb)
            {
            	DLOGI << "SRGB format." << std::endl;
            }
	    	if(spec.compression == TextureCompression::DXT5)
	    	{
	    		DLOGI << "DXT5 compression." << std::endl;
	    	}

            texture_maps.insert(std::make_pair(tmap_hname, tmap));
        }
    }

    // * Group texture maps where possible
    if(s_allow_grouping)
    {
    	if(handle_groups(texture_maps, width, height))
    	{
    		DLOG("fudge",1) << "New texture map composition: " << std::endl;
    		for(auto&& [key,dat]: texture_maps)
    		{
    			DLOGI << dat.name << std::endl;
    		}
    	}
    }

    // * Compress textures if needed
    // Handled in tom_file
    /*for(auto&& [key, tmap]: texture_maps)
    {
	    if(tmap.compression == TextureCompression::DXT5)
	    {
			uint8_t* compressed = fudge::compress_dxt_5(tmap.data, width, height);
			stbi_image_free(tmap.data);
			// Reassign
			tmap.data = compressed;
	    }
	}*/

    // * Write TOM file
	std::string out_file_name = input_dir.stem().string() + ".tom";
	tom::TOMDescriptor tom_desc
	{
		output_dir / out_file_name,
		(uint16_t)width,
		(uint16_t)height,
		((s_blob_compression==BlobCompression::Deflate) ? tom::LosslessCompression::Deflate : tom::LosslessCompression::None),
		TextureWrap::REPEAT
	};

    // Find a matching layout, if found, strictly follow slot order
    std::vector<std::pair<hash_t, TexmapData*>> ordered_tmap;
    const LayoutSpec* layout = find_layout(texture_maps);
    if(layout == nullptr)
    {
    	DLOGW("fudge") << "Cannot find a valid material layout for this collection." << std::endl;
    	for(auto&& [key, tmap]: texture_maps)
    		ordered_tmap.push_back(std::make_pair(key,&tmap));
    }
    else
    {
    	DLOG("fudge",1) << "Detected material layout: " << WCC('n') << layout->name << std::endl;
    	for(auto&& hslot: layout->slots)
    		ordered_tmap.push_back(std::make_pair(hslot, &texture_maps[hslot]));

	    DLOG("fudge",1) << "Slots:" << std::endl;
		for(int ii=0; ii<ordered_tmap.size(); ++ii)
		{
			DLOGI << ii << ": " << ordered_tmap[ii].second->name << std::endl;
		}
    }

	// Construct and push texture map descriptors
	for(auto&& [key, tmap]: ordered_tmap)
	{
		const TexmapSpec& spec = s_texmap_specs.at(key);
		uint32_t size = width * height * spec.channels;
		if(tmap->compression == TextureCompression::DXT5)
			size = width * height;
		tom::TextureMapDescriptor tm_desc
		{
			spec.filter,
			(uint8_t)spec.channels,
			spec.srgb,
			spec.compression,
			size,
			tmap->data,
			key
		};

		tom_desc.texture_maps.push_back(tm_desc);
	}

	DLOG("fudge",1) << "Exporting: " << WCC('p') << out_file_name << std::endl;
	tom::write_tom(tom_desc);

    // Cleanup
    for(auto&& [key, tmap]: texture_maps)
        stbi_image_free(tmap.data);
}

} // namespace texmap
} // namespace fudge