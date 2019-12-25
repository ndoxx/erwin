#include "core/value_map.h"
#include "utils/string.h"
#include "filesystem/filesystem.h"
#include "filesystem/xml_file.h"

namespace erwin
{

void ValueMap::init(void* xml_root_node, const std::string& rootname)
{
	if(xml_root_node)
		parse_properties(static_cast<rapidxml::xml_node<>*>(xml_root_node), rootname);
}

// Property parser
hash_t ValueMap::parse_xml_property(void* node, const std::string& name_chain)
{
	rapidxml::xml_node<>* xnode = static_cast<rapidxml::xml_node<>*>(node);

    std::string str_var_name;
    if(!xml::parse_attribute(xnode, "name", str_var_name))
        return 0;

    std::string str_full_name(name_chain + "." + str_var_name);
    hash_t full_name_hash = H_(str_full_name.c_str());

    // Get hash from node name
    hash_t nameHash = H_(xnode->name());

    switch(nameHash)
    {
        case "uint"_h:
        {
            uint32_t value = 0;
            if(!xml::parse_attribute(xnode, "value", value)) return 0;
            uints_[full_name_hash] = value;
            break;
        }
        case "int"_h:
        {
            int32_t value = 0;
            if(!xml::parse_attribute(xnode, "value", value)) return 0;
            ints_[full_name_hash] = value;
            break;
        }
        case "float"_h:
        {
            float value = 0.0f;
            if(!xml::parse_attribute(xnode, "value", value)) return 0;
            floats_[full_name_hash] = value;
            break;
        }
        case "bool"_h:
        {
            bool value = false;
            if(!xml::parse_attribute(xnode, "value", value)) return 0;
            bools_[full_name_hash] = value;
            break;
        }
        case "string"_h:
        {
            std::string value;
            if(!xml::parse_attribute(xnode, "value", value)) return 0;
            strings_[full_name_hash] = value;
            break;
        }
        case "vec2"_h:
        {
            glm::vec2 value(0.0f);
            if(!xml::parse_attribute(xnode, "value", value)) return 0;
            vec2s_[full_name_hash] = value;
            break;
        }
        case "vec3"_h:
        {
            glm::vec3 value(0.0f);
            if(!xml::parse_attribute(xnode, "value", value)) return 0;
            vec3s_[full_name_hash] = value;
            break;
        }
        case "vec4"_h:
        {
            glm::vec4 value(0.0f);
            if(!xml::parse_attribute(xnode, "value", value)) return 0;
            vec4s_[full_name_hash] = value;
            break;
        }
        case "path"_h:
        {
            std::string value;
            if(!xml::parse_attribute(xnode, "value", value)) return 0;
            fs::path the_path(filesystem::get_root_dir() / value);
            if(!fs::exists(the_path)) return 0;
            paths_[full_name_hash] = the_path;
            break;
        }
        case "size"_h:
        {
            std::string value;
            if(!xml::parse_attribute(xnode, "value", value)) return 0;
            sizes_[full_name_hash] = su::parse_size(value, '_');
            break;
        }
    }

    return full_name_hash;
}

// Recursive parser
void ValueMap::parse_properties(void* node, const std::string& name_chain)
{
	rapidxml::xml_node<>* xnode = static_cast<rapidxml::xml_node<>*>(node);

    // For each siblings at this recursion level
    for(rapidxml::xml_node<>* cur_node=xnode->first_node();
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

template <> size_t ValueMap::get(hash_t hname, size_t def)
{
    auto it = sizes_.find(hname);
    return (it != sizes_.end()) ? it->second : def;
}

template <> uint32_t ValueMap::get(hash_t hname, uint32_t def)
{
	auto it = uints_.find(hname);
	return (it != uints_.end()) ? it->second : def;
}

template <> int32_t ValueMap::get(hash_t hname, int32_t def)
{
	auto it = ints_.find(hname);
	return (it != ints_.end()) ? it->second : def;
}

template <> float ValueMap::get(hash_t hname, float def)
{
	auto it = floats_.find(hname);
	return (it != floats_.end()) ? it->second : def;
}

template <> bool ValueMap::get(hash_t hname, bool def)
{
	auto it = bools_.find(hname);
	return (it != bools_.end()) ? it->second : def;
}

template <> std::string ValueMap::get(hash_t hname, std::string def)
{
	auto it = strings_.find(hname);
	return (it != strings_.end()) ? it->second : def;
}

template <> glm::vec2 ValueMap::get(hash_t hname, glm::vec2 def)
{
	auto it = vec2s_.find(hname);
	return (it != vec2s_.end()) ? it->second : def;
}

template <> glm::vec3 ValueMap::get(hash_t hname, glm::vec3 def)
{
	auto it = vec3s_.find(hname);
	return (it != vec3s_.end()) ? it->second : def;
}

template <> glm::vec4 ValueMap::get(hash_t hname, glm::vec4 def)
{
	auto it = vec4s_.find(hname);
	return (it != vec4s_.end()) ? it->second : def;
}

fs::path ValueMap::get(hash_t hname)
{
	auto it = paths_.find(hname);
	return (it != paths_.end()) ? it->second : fs::path();
}

bool ValueMap::is(hash_t name)
{
    auto it = bools_.find(name);
	return (it != bools_.end()) ? it->second : false;
}

} // namespace erwin