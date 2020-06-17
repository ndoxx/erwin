#include "core/config.h"
#include "utils/string.h"
#include "core/registry.h"
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

static Registry registry;

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
		WLOGGER(create_channel(chan_name, uint8_t(chan_verbosity)));
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
	    		su::tokenize(sink_chan_list, ',', [&](const std::string& chan_name)
	    		{
	    			chan_hnames.push_back(H_(chan_name.c_str()));
	    		});
    			attach_all = false;
    		}
    	}

    	if(attach_all)
    	{
			WLOGGER(attach_all(sink_name, std::move(p_sink)));
    	}
    	else if(!chan_hnames.empty())
    	{
			WLOGGER(attach(sink_name, std::move(p_sink), chan_hnames));
    	}
    }
}

bool load(const fs::path& filepath)
{
	xml::XMLFile cfg_f(filepath);
	if(!cfg_f.read())
		return false;

	registry.deserialize(cfg_f);

	// Special treatment for logger configuration node if any
	auto* logger_node = cfg_f.root->first_node("logger");
	if(logger_node)
		init_logger(logger_node);

	return true;
}

bool save(const fs::path& filepath)
{
	xml::XMLFile cfg_f(filepath);
	if(!cfg_f.read())
		return false;

	registry.serialize(cfg_f);
	cfg_f.write();

	return true;
}

template <> const size_t& get(hash_t hname, const size_t& def)
{
	return registry.get(hname, def);
}

template <> const uint32_t& get(hash_t hname, const uint32_t& def)
{
	return registry.get(hname, def);
}

template <> const int32_t& get(hash_t hname, const int32_t& def)
{
	return registry.get(hname, def);
}

template <> const float& get(hash_t hname, const float& def)
{
	return registry.get(hname, def);
}

template <> const bool& get(hash_t hname, const bool& def)
{
	return registry.get(hname, def);
}

template <> const std::string& get(hash_t hname, const std::string& def)
{
	return registry.get(hname, def);
}

template <> const glm::vec2& get(hash_t hname, const glm::vec2& def)
{
	return registry.get(hname, def);
}

template <> const glm::vec3& get(hash_t hname, const glm::vec3& def)
{
	return registry.get(hname, def);
}

template <> const glm::vec4& get(hash_t hname, const glm::vec4& def)
{
	return registry.get(hname, def);
}

const fs::path& get(hash_t hname)
{
	return registry.get(hname);
}

hash_t get_hash(hash_t hname, const std::string& def)
{
	return H_(registry.get(hname, def).c_str());
}

hash_t get_hash_lower(hash_t hname, const std::string& def)
{
	std::string str(registry.get(hname, def));
    su::to_lower(str);
    return H_(str.c_str());
}

hash_t get_hash_upper(hash_t hname, const std::string& def)
{
	std::string str(registry.get(hname, def));
    su::to_upper(str);
    return H_(str.c_str());
}

template <> bool set(hash_t hname, const size_t& val)
{
	return registry.set(hname, val);
}

template <> bool set(hash_t hname, const uint32_t& val)
{
	return registry.set(hname, val);
}

template <> bool set(hash_t hname, const int32_t& val)
{
	return registry.set(hname, val);
}

template <> bool set(hash_t hname, const float& val)
{
	return registry.set(hname, val);
}

template <> bool set(hash_t hname, const bool& val)
{
	return registry.set(hname, val);
}

template <> bool set(hash_t hname, const std::string& val)
{
	return registry.set(hname, val);
}

template <> bool set(hash_t hname, const fs::path& val)
{
	return registry.set(hname, val);
}

template <> bool set(hash_t hname, const glm::vec2& val)
{
	return registry.set(hname, val);
}

template <> bool set(hash_t hname, const glm::vec3& val)
{
	return registry.set(hname, val);
}

template <> bool set(hash_t hname, const glm::vec4& val)
{
	return registry.set(hname, val);
}


} // namespace cfg
} // namespace erwin