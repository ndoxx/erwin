#pragma once

#include <sstream>
#include <functional>
#include <filesystem>
#include <memory>
#include <cstring>
#include <vector>

#include "utils/string.h"
#include "glm/glm.hpp"
#include "rapidxml/rapidxml.hpp"

namespace fs = std::filesystem;

namespace erwin
{

template<> std::string to_string(const glm::vec2& v);
template<> std::string to_string(const glm::vec3& v);
template<> std::string to_string(const glm::vec4& v);
template<> std::string to_string(const glm::ivec2& v);
template<> std::string to_string(const glm::ivec3& v);
template<> std::string to_string(const glm::ivec4& v);

template <typename T>
bool str_val(const char* value, T& result)
{
    std::istringstream iss(value);
    return !(iss >> result).fail();
}

// Full specializations
template <>
bool str_val<glm::vec2>(const char* value, glm::vec2& result);

template <>
bool str_val<glm::vec3>(const char* value, glm::vec3& result);

template <>
bool str_val<glm::vec4>(const char* value, glm::vec4& result);

template <>
bool str_val<glm::ivec2>(const char* value, glm::ivec2& result);

template <>
bool str_val<glm::ivec3>(const char* value, glm::ivec3& result);

template <>
bool str_val<glm::ivec4>(const char* value, glm::ivec4& result);

template <>
bool str_val<bool>(const char* value, bool& result);

namespace xml
{

struct XMLFile
{
    fs::path filepath;
    rapidxml::xml_document<> doc;
    rapidxml::xml_node<>* root = nullptr;
    std::string buffer;

    XMLFile() = default;
    explicit XMLFile(const fs::path& filepath): filepath(filepath), root(nullptr) { }
    ~XMLFile() { release(); }

    // Read and parse an XML file. Only the filepath member need be initialized.
    bool read();
    // Write an XML file, filepath, buffer and doc must be initialized
    void write();

    void release();
    void create_root(const char* root_name, bool write_declaration = true);
    rapidxml::xml_node<>* add_node(rapidxml::xml_node<>* parent, const char* node_name, const char* node_value = nullptr);
    rapidxml::xml_attribute<>* add_attribute(rapidxml::xml_node<>* node, const char* attr_name, const char* attr_val);
    void set_value(rapidxml::xml_node<>* node, const char* value);
};

template <typename T>
void parse_attribute(rapidxml::xml_node<>* node, const char* name, std::function<void(T value)> exec)
{
    T value;
    rapidxml::xml_attribute<>* pAttr = node->first_attribute(name);
    if(!pAttr) return;

    if(str_val(pAttr->value(), value))
        exec(value);
}

template <typename T>
void parse_node(rapidxml::xml_node<>* parent, const char* name, std::function<void(T value)> exec)
{
    rapidxml::xml_node<>* leaf_node = parent->first_node(name);
    if(!leaf_node) return;

    T value;
    bool success = str_val(leaf_node->value(), value);
    if(success)
        exec(value);
}

template <typename T>
bool parse_attribute(rapidxml::xml_node<>* node, const char* name, T& destination)
{
    rapidxml::xml_attribute<>* pAttr = node->first_attribute(name);
    if(!pAttr)
        return false;
    return str_val(node->first_attribute(name)->value(), destination);
}

template <typename T>
bool set_attribute(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node, const char* name, const T& source)
{
    rapidxml::xml_attribute<>* pAttr = node->first_attribute(name);
    if(!pAttr)
        return false;

    char* attr_value = doc.allocate_string(to_string<T>(source).c_str());
    pAttr->value(attr_value);
    return true;
}

template <typename T>
bool parse_node(rapidxml::xml_node<>* parent, const char* leaf_name, T& destination)
{
    rapidxml::xml_node<>* leaf_node = parent->first_node(leaf_name);
    if(!leaf_node)
        return false;

    return str_val(leaf_node->value(), destination);
}

bool parse_attribute(rapidxml::xml_node<>* node, const char* name, std::string& destination);
hash_t parse_attribute_h(rapidxml::xml_node<>* node, const char* name);
bool parse_node(rapidxml::xml_node<>* parent, const char* leaf_name, std::string& destination);
hash_t parse_node_h(rapidxml::xml_node<>* parent, const char* leaf_name);

template <>
void parse_node<const char*>(rapidxml::xml_node<>* parent, const char* name, std::function<void(const char* value)> exec);

template <>
bool set_attribute(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node, const char* name, const std::string& source);

// Unused
template <typename T>
bool parse_property(rapidxml::xml_node<>* parent, const char* prop_name, T& destination)
{
    for (rapidxml::xml_node<>* prop=parent->first_node("Prop");
         prop; prop=prop->next_sibling("Prop"))
    {
        rapidxml::xml_attribute<>* pAttr = prop->first_attribute("name");
        if(!pAttr) continue;
        const char* propertyName = pAttr->value();

        if(!strcmp(propertyName, prop_name))
            return str_val(prop->value(), destination);
    }
    return false;
}

} // namespace xml
} // namespace erwin
