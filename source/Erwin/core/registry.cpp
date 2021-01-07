#include "core/registry.h"
#include "core/application.h"
#include <kibble/logger/logger.h>
#include <kibble/string/string.h>

namespace erwin
{

void Registry::deserialize(const xml::XMLFile& xml, const std::string& root_name)
{
    // Use file name as root name if no root name specified
    std::string root = root_name;
    if(root.empty())
        root = WFS().regular_path(xml.filepath).stem();

    if(xml.root)
        parse_properties(xml.root, root, WFS().regular_path(xml.filepath).parent_path());
}

// Recursive parser
void Registry::parse_properties(void* node, const std::string& name_chain, const fs::path& parent_dir)
{
    rapidxml::xml_node<>* xnode = static_cast<rapidxml::xml_node<>*>(node);

    // For each siblings at this recursion level
    for(rapidxml::xml_node<>* cur_node = xnode->first_node(); cur_node; cur_node = cur_node->next_sibling())
    {
        // Look for children node if any
        rapidxml::xml_node<>* child_node = cur_node->first_node();
        if(child_node)
        {
            // Get current node name and append to chain
            const char* node_name = cur_node->name();
            std::string chain(name_chain + "." + node_name);
            // Get configuration for next level
            parse_properties(cur_node, chain, parent_dir);
        }
        else
        {
            // If no child, then try to extract property
            hash_t name_hash = parse_xml_property(cur_node, name_chain, parent_dir);
            if(!name_hash)
            {
                // Node is invalid
            }
        }
    }
}

// Property parser
hash_t Registry::parse_xml_property(void* node, const std::string& name_chain, const fs::path& /*parent_dir*/)
{
    rapidxml::xml_node<>* xnode = static_cast<rapidxml::xml_node<>*>(node);

    std::string str_var_name;
    if(!xml::parse_attribute(xnode, "name", str_var_name))
        return 0;

    std::string str_full_name(name_chain + "." + str_var_name);
    hash_t full_name_hash = H_(str_full_name.c_str());

    // Get hash from node name
    hash_t type_h = H_(xnode->name());

    switch(type_h)
    {
    case "uint"_h: {
        uint32_t value = 0;
        if(!xml::parse_attribute(xnode, "value", value))
            return 0;
        uints_[full_name_hash] = value;
        break;
    }
    case "int"_h: {
        int32_t value = 0;
        if(!xml::parse_attribute(xnode, "value", value))
            return 0;
        ints_[full_name_hash] = value;
        break;
    }
    case "float"_h: {
        float value = 0.0f;
        if(!xml::parse_attribute(xnode, "value", value))
            return 0;
        floats_[full_name_hash] = value;
        break;
    }
    case "bool"_h: {
        bool value = false;
        if(!xml::parse_attribute(xnode, "value", value))
            return 0;
        bools_[full_name_hash] = value;
        break;
    }
    case "string"_h: {
        std::string value;
        if(!xml::parse_attribute(xnode, "value", value))
            return 0;
        strings_[full_name_hash] = value;
        break;
    }
    case "vec2"_h: {
        glm::vec2 value(0.0f);
        if(!xml::parse_attribute(xnode, "value", value))
            return 0;
        vec2s_[full_name_hash] = value;
        break;
    }
    case "vec3"_h: {
        glm::vec3 value(0.0f);
        if(!xml::parse_attribute(xnode, "value", value))
            return 0;
        vec3s_[full_name_hash] = value;
        break;
    }
    case "vec4"_h: {
        glm::vec4 value(0.0f);
        if(!xml::parse_attribute(xnode, "value", value))
            return 0;
        vec4s_[full_name_hash] = value;
        break;
    }
    case "path"_h: {
        std::string value;
        if(!xml::parse_attribute(xnode, "value", value))
            return 0;

        strings_[full_name_hash] = std::string(value);
        break;
    }
    case "size"_h: {
        std::string value;
        if(!xml::parse_attribute(xnode, "value", value))
            return 0;
        sizes_[full_name_hash] = kb::su::parse_size(value, '_');
        break;
    }
    }

    return full_name_hash;
}

void Registry::serialize(xml::XMLFile& xml, const std::string& root_name)
{
    // Use file name as root name if no root name specified
    std::string root = root_name;
    if(root.empty())
        root = WFS().regular_path(xml.filepath).stem();

    if(xml.root)
        serialize_properties(&xml.doc, xml.root, root, WFS().regular_path(xml.filepath).parent_path());
}

void Registry::serialize_properties(void* pdoc, void* node, const std::string& name_chain, const fs::path& parent_dir)
{
    rapidxml::xml_node<>* xnode = static_cast<rapidxml::xml_node<>*>(node);

    // For each siblings at this recursion level
    for(rapidxml::xml_node<>* cur_node = xnode->first_node(); cur_node; cur_node = cur_node->next_sibling())
    {
        // Look for children node if any
        rapidxml::xml_node<>* child_node = cur_node->first_node();
        if(child_node)
        {
            // Get current node name and append to chain
            const char* node_name = cur_node->name();
            std::string chain(name_chain + "." + node_name);
            // Get configuration for next level
            serialize_properties(pdoc, cur_node, chain, parent_dir);
        }
        else
        {
            // If no child, then try to write property
            write_xml_property(pdoc, cur_node, name_chain, parent_dir);
        }
    }
}

void Registry::clear()
{
    sizes_.clear();
    uints_.clear();
    ints_.clear();
    floats_.clear();
    bools_.clear();
    strings_.clear();
    vec2s_.clear();
    vec3s_.clear();
    vec4s_.clear();
}

void Registry::write_xml_property(void* pdoc, void* node, const std::string& name_chain, const fs::path& /*parent_dir*/)
{
    rapidxml::xml_node<>* xnode = static_cast<rapidxml::xml_node<>*>(node);

    std::string str_var_name;
    if(!xml::parse_attribute(xnode, "name", str_var_name))
        return;

    std::string str_full_name(name_chain + "." + str_var_name);
    hash_t full_name_hash = H_(str_full_name.c_str());

    // Get hash from node name
    hash_t type_h = H_(xnode->name());

    auto& doc = *static_cast<rapidxml::xml_document<>*>(pdoc);
    switch(type_h)
    {
    case "uint"_h: {
        auto it = uints_.find(full_name_hash);
        if(it != uints_.end())
            xml::set_attribute(doc, xnode, "value", it->second);
        break;
    }
    case "int"_h: {
        auto it = ints_.find(full_name_hash);
        if(it != ints_.end())
            xml::set_attribute(doc, xnode, "value", it->second);
        break;
    }
    case "float"_h: {
        auto it = floats_.find(full_name_hash);
        if(it != floats_.end())
            xml::set_attribute(doc, xnode, "value", it->second);
        break;
    }
    case "bool"_h: {
        auto it = bools_.find(full_name_hash);
        if(it != bools_.end())
            xml::set_attribute(doc, xnode, "value", std::string(it->second ? "true" : "false"));
        break;
    }
    case "string"_h: {
        auto it = strings_.find(full_name_hash);
        if(it != strings_.end())
            xml::set_attribute(doc, xnode, "value", it->second);
        break;
    }
    case "vec2"_h: {
        auto it = vec2s_.find(full_name_hash);
        if(it != vec2s_.end())
            xml::set_attribute(doc, xnode, "value", it->second);
        break;
    }
    case "vec3"_h: {
        auto it = vec3s_.find(full_name_hash);
        if(it != vec3s_.end())
            xml::set_attribute(doc, xnode, "value", it->second);
        break;
    }
    case "vec4"_h: {
        auto it = vec4s_.find(full_name_hash);
        if(it != vec4s_.end())
            xml::set_attribute(doc, xnode, "value", it->second);
        break;
    }
    case "path"_h: {
        auto it = strings_.find(full_name_hash);
        if(it != strings_.end())
        {
            if(!it->second.empty())
                xml::set_attribute(doc, xnode, "value", it->second);
        }
        break;
    }
    case "size"_h: {
        auto it = sizes_.find(full_name_hash);
        if(it != sizes_.end())
            xml::set_attribute(doc, xnode, "value", kb::su::size_to_string(it->second));
        break;
    }
    }
}

template <> const size_t& Registry::get(hash_t hname, const size_t& def) const
{
    auto it = sizes_.find(hname);
    return (it != sizes_.end()) ? it->second : def;
}

template <> const uint32_t& Registry::get(hash_t hname, const uint32_t& def) const
{
    auto it = uints_.find(hname);
    return (it != uints_.end()) ? it->second : def;
}

template <> const int32_t& Registry::get(hash_t hname, const int32_t& def) const
{
    auto it = ints_.find(hname);
    return (it != ints_.end()) ? it->second : def;
}

template <> const float& Registry::get(hash_t hname, const float& def) const
{
    auto it = floats_.find(hname);
    return (it != floats_.end()) ? it->second : def;
}

template <> const bool& Registry::get(hash_t hname, const bool& def) const
{
    auto it = bools_.find(hname);
    return (it != bools_.end()) ? it->second : def;
}

template <> const std::string& Registry::get(hash_t hname, const std::string& def) const
{
    auto it = strings_.find(hname);
    return (it != strings_.end()) ? it->second : def;
}

template <> const glm::vec2& Registry::get(hash_t hname, const glm::vec2& def) const
{
    auto it = vec2s_.find(hname);
    return (it != vec2s_.end()) ? it->second : def;
}

template <> const glm::vec3& Registry::get(hash_t hname, const glm::vec3& def) const
{
    auto it = vec3s_.find(hname);
    return (it != vec3s_.end()) ? it->second : def;
}

template <> const glm::vec4& Registry::get(hash_t hname, const glm::vec4& def) const
{
    auto it = vec4s_.find(hname);
    return (it != vec4s_.end()) ? it->second : def;
}

static std::string empty_path;
const std::string& Registry::get(hash_t hname) const
{
    auto it = strings_.find(hname);
    return (it != strings_.end()) ? it->second : empty_path;
}

bool Registry::is(hash_t name) const
{
    auto it = bools_.find(name);
    return (it != bools_.end()) ? it->second : false;
}

template <> bool Registry::set(hash_t hname, const size_t& val)
{
    auto it = sizes_.find(hname);
    if(it == sizes_.end())
        return false;
    it->second = val;
    return true;
}

template <> bool Registry::set(hash_t hname, const uint32_t& val)
{
    auto it = uints_.find(hname);
    if(it == uints_.end())
        return false;
    it->second = val;
    return true;
}

template <> bool Registry::set(hash_t hname, const int32_t& val)
{
    auto it = ints_.find(hname);
    if(it == ints_.end())
        return false;
    it->second = val;
    return true;
}

template <> bool Registry::set(hash_t hname, const float& val)
{
    auto it = floats_.find(hname);
    if(it == floats_.end())
        return false;
    it->second = val;
    return true;
}

template <> bool Registry::set(hash_t hname, const bool& val)
{
    auto it = bools_.find(hname);
    if(it == bools_.end())
        return false;
    it->second = val;
    return true;
}

template <> bool Registry::set(hash_t hname, const std::string& val)
{
    auto it = strings_.find(hname);
    if(it == strings_.end())
        return false;
    it->second = val;
    return true;
}

template <> bool Registry::set(hash_t hname, const glm::vec2& val)
{
    auto it = vec2s_.find(hname);
    if(it == vec2s_.end())
        return false;
    it->second = val;
    return true;
}

template <> bool Registry::set(hash_t hname, const glm::vec3& val)
{
    auto it = vec3s_.find(hname);
    if(it == vec3s_.end())
        return false;
    it->second = val;
    return true;
}

template <> bool Registry::set(hash_t hname, const glm::vec4& val)
{
    auto it = vec4s_.find(hname);
    if(it == vec4s_.end())
        return false;
    it->second = val;
    return true;
}

} // namespace erwin