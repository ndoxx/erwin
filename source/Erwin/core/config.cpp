#include "core/config.h"
#include "core/core.h"
#include "core/string_utils.h"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "filesystem/xml_file.h"

#include <unordered_map>
#include <string>
#include <iostream>

namespace erwin
{
namespace cfg
{

static std::unordered_map<hash_t, uint32_t>    s_uints;
static std::unordered_map<hash_t, int32_t>     s_ints;
static std::unordered_map<hash_t, float>       s_floats;
static std::unordered_map<hash_t, bool>        s_bools;
static std::unordered_map<hash_t, std::string> s_strings;
static std::unordered_map<hash_t, glm::vec2>   s_vec2s;
static std::unordered_map<hash_t, glm::vec3>   s_vec3s;
static std::unordered_map<hash_t, glm::vec4>   s_vec4s;
static std::unordered_map<hash_t, fs::path>    s_paths;

static hash_t parse_xml_property(rapidxml::xml_node<>* node,
                                 const std::string& name_chain)
{
    std::string str_var_name;
    if(!xml::parse_attribute(node, "name", str_var_name))
        return 0;

    std::string str_full_name(name_chain + "." + str_var_name);
    hash_t full_name_hash = H_(str_full_name.c_str());

    // Get hash from node name
    hash_t nameHash = H_(node->name());

    switch(nameHash)
    {
        case "uint"_h:
        {
            uint32_t value = 0;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            s_uints[full_name_hash] = value;
            break;
        }
        case "int"_h:
        {
            int32_t value = 0;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            s_ints[full_name_hash] = value;
            break;
        }
        case "float"_h:
        {
            float value = 0.0f;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            s_floats[full_name_hash] = value;
            break;
        }
        case "bool"_h:
        {
            bool value = false;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            s_bools[full_name_hash] = value;
            break;
        }
        case "string"_h:
        {
            std::string value;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            s_strings[full_name_hash] = value;
            break;
        }
        case "vec2"_h:
        {
            glm::vec2 value(0.0f);
            if(!xml::parse_attribute(node, "value", value)) return 0;
            s_vec2s[full_name_hash] = value;
            break;
        }
        case "vec3"_h:
        {
            glm::vec3 value(0.0f);
            if(!xml::parse_attribute(node, "value", value)) return 0;
            s_vec3s[full_name_hash] = value;
            break;
        }
        case "vec4"_h:
        {
            glm::vec4 value(0.0f);
            if(!xml::parse_attribute(node, "value", value)) return 0;
            s_vec4s[full_name_hash] = value;
            break;
        }
        case "path"_h:
        {
            std::string value;
            if(!xml::parse_attribute(node, "value", value)) return 0;
            fs::path the_path(filesystem::get_root_dir() / value);
            if(!fs::exists(the_path)) return 0;
            s_paths[full_name_hash] = the_path;
            break;
        }
    }

    return full_name_hash;
}

// Recursive parser
static void parse_properties(rapidxml::xml_node<>* node,
                             const std::string& name_chain)
{
    // For each siblings at this recursion level
    for(rapidxml::xml_node<>* cur_node=node->first_node();
        cur_node;
        cur_node=cur_node->next_sibling())
    {
        // Look for children node if any
        rapidxml::xml_node<>* child_node = cur_node->first_node();
        if(child_node)
        {
            // Get current node name and append to chain
            const char* node_name = cur_node->name();
            std::string chain(name_chain + "." + node_name);
            // Get configuration for next level
            parse_properties(cur_node, chain);
        }
        else
        {
            // If no child, then try to extract property
            hash_t name_hash = parse_xml_property(cur_node, name_chain);
            if(!name_hash)
            {
                // Node is invalid
            }
        }
    }
}

static void init_logger(rapidxml::xml_node<>* node)
{
	// Add and configure channels
    for(rapidxml::xml_node<>* chan=node->first_node("Channel");
        chan; chan=chan->next_sibling("Channel"))
    {
    	std::string chan_name;
    	uint32_t chan_verbosity=0;
		if(!xml::parse_attribute(chan, "name", chan_name)) continue;
		xml::parse_attribute(chan, "verbosity", chan_verbosity);
		WLOGGER.create_channel(chan_name, chan_verbosity);
	}

	// Add and configure sinks
    for(rapidxml::xml_node<>* sink=node->first_node("Sink");
        sink; sink=sink->next_sibling("Sink"))
    {
    	std::string sink_name;
    	std::string sink_type;
    	std::string sink_chan_list;
		if(!xml::parse_attribute(sink, "name", sink_name)) continue;
		if(!xml::parse_attribute(sink, "type", sink_type)) continue;

    	// Sink creation
		hash_t htype = H_(sink_type.c_str());
		std::unique_ptr<dbg::Sink> p_sink = nullptr;
		switch(htype)
		{
			case "ConsoleSink"_h:
			{
				p_sink = std::make_unique<dbg::ConsoleSink>();
				break;
			}
			case "LogFileSink"_h:
			{
				std::string dest_file;
				if(xml::parse_attribute(sink, "destination", dest_file))
					p_sink = std::make_unique<dbg::LogFileSink>(dest_file);
				break;
			}
		}

		if(!p_sink) continue;

		// Attachments
		bool attach_all = true;
		std::vector<hash_t> chan_hnames;
    	if(xml::parse_attribute(sink, "channels", sink_chan_list))
    	{
    		if(!sink_chan_list.compare("all"))
    			attach_all = true;
    		else
    		{
	    		tokenize(sink_chan_list, ',', [&](const std::string& chan_name)
	    		{
	    			chan_hnames.push_back(H_(chan_name.c_str()));
	    		});
    			attach_all = false;
    		}
    	}

    	if(attach_all)
    	{
			WLOGGER.attach_all(sink_name, std::move(p_sink));
    	}
    	else if(!chan_hnames.empty())
    	{
			WLOGGER.attach(sink_name, std::move(p_sink), chan_hnames);
    	}
    }
}

bool init(const fs::path& filepath)
{
	xml::XMLFile cfg_f(filepath);
	if(!cfg_f.read())
	{
		return false;
	}

	parse_properties(cfg_f.root, "erwin");
	init_logger(cfg_f.root->first_node("logger"));

	return true;
}

template <> uint32_t get(hash_t hname, uint32_t def)
{
	auto it = s_uints.find(hname);
	if(it != s_uints.end())
		return it->second;
	return def;
}

template <> int32_t get(hash_t hname, int32_t def)
{
	auto it = s_ints.find(hname);
	if(it != s_ints.end())
		return it->second;
	return def;
}

template <> float get(hash_t hname, float def)
{
	auto it = s_floats.find(hname);
	if(it != s_floats.end())
		return it->second;
	return def;
}

template <> bool get(hash_t hname, bool def)
{
	auto it = s_bools.find(hname);
	if(it != s_bools.end())
		return it->second;
	return def;
}

template <> std::string get(hash_t hname, std::string def)
{
	auto it = s_strings.find(hname);
	if(it != s_strings.end())
		return it->second;
	return def;
}

template <> glm::vec2 get(hash_t hname, glm::vec2 def)
{
	auto it = s_vec2s.find(hname);
	if(it != s_vec2s.end())
		return it->second;
	return def;
}

template <> glm::vec3 get(hash_t hname, glm::vec3 def)
{
	auto it = s_vec3s.find(hname);
	if(it != s_vec3s.end())
		return it->second;
	return def;
}

template <> glm::vec4 get(hash_t hname, glm::vec4 def)
{
	auto it = s_vec4s.find(hname);
	if(it != s_vec4s.end())
		return it->second;
	return def;
}

fs::path get(hash_t hname)
{
	auto it = s_paths.find(hname);
	if(it != s_paths.end())
		return it->second;
	return fs::path();
}

} // namespace cfg
} // namespace erwin