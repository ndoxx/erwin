#include "core/config.h"
#include "core/core.h"
#include "core/string_utils.h"
#include "core/value_map.h"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"
#include "filesystem/xml_file.h"

#include <unordered_map>
#include <string>
#include <iostream>

namespace erwin
{
namespace cfg
{

static ValueMap vmap;

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

	vmap.init(cfg_f.root, "erwin");
	init_logger(cfg_f.root->first_node("logger"));

	return true;
}

template <> size_t get(hash_t hname, size_t def)
{
	return vmap.get(hname, def);
}

template <> uint32_t get(hash_t hname, uint32_t def)
{
	return vmap.get(hname, def);
}

template <> int32_t get(hash_t hname, int32_t def)
{
	return vmap.get(hname, def);
}

template <> float get(hash_t hname, float def)
{
	return vmap.get(hname, def);
}

template <> bool get(hash_t hname, bool def)
{
	return vmap.get(hname, def);
}

template <> std::string get(hash_t hname, std::string def)
{
	return vmap.get(hname, def);
}

template <> glm::vec2 get(hash_t hname, glm::vec2 def)
{
	return vmap.get(hname, def);
}

template <> glm::vec3 get(hash_t hname, glm::vec3 def)
{
	return vmap.get(hname, def);
}

template <> glm::vec4 get(hash_t hname, glm::vec4 def)
{
	return vmap.get(hname, def);
}

fs::path get(hash_t hname)
{
	return vmap.get(hname);
}

} // namespace cfg
} // namespace erwin