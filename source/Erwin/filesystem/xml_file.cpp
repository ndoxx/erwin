#include "filesystem/xml_file.h"
#include "filesystem/filesystem.h"
#include "rapidxml/rapidxml_print.hpp"
#include <kibble/logger/logger.h>

using namespace rapidxml;


namespace erwin
{

template <> std::string to_string<glm::vec2>(const glm::vec2& v)
{
    return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + ")";
}

template <> std::string to_string<glm::vec3>(const glm::vec3& v)
{
    return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + ")";
}

template <> std::string to_string<glm::vec4>(const glm::vec4& v)
{
    return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + "," +
           std::to_string(v.w) + ")";
}

template <> std::string to_string<glm::ivec2>(const glm::ivec2& v)
{
    return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + ")";
}

template <> std::string to_string<glm::ivec3>(const glm::ivec3& v)
{
    return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + ")";
}

template <> std::string to_string<glm::ivec4>(const glm::ivec4& v)
{
    return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + "," +
           std::to_string(v.w) + ")";
}

template <> bool str_val<glm::vec2>(const char* value, glm::vec2& result)
{
    return sscanf(value, "(%f,%f)", &result[0], &result[1]) > 0;
}

template <> bool str_val<glm::vec3>(const char* value, glm::vec3& result)
{
    return sscanf(value, "(%f,%f,%f)", &result[0], &result[1], &result[2]) > 0;
}

template <> bool str_val<glm::vec4>(const char* value, glm::vec4& result)
{
    return sscanf(value, "(%f,%f,%f,%f)", &result[0], &result[1], &result[2], &result[3]) > 0;
}

template <> bool str_val<glm::ivec2>(const char* value, glm::ivec2& result)
{
    return sscanf(value, "(%d,%d)", &result[0], &result[1]) > 0;
}

template <> bool str_val<glm::ivec3>(const char* value, glm::ivec3& result)
{
    return sscanf(value, "(%d,%d,%d)", &result[0], &result[1], &result[2]) > 0;
}

template <> bool str_val<glm::ivec4>(const char* value, glm::ivec4& result)
{
    return sscanf(value, "(%d,%d,%d,%d)", &result[0], &result[1], &result[2], &result[3]) > 0;
}

template <> bool str_val<bool>(const char* value, bool& result)
{
    result = !strcmp(value, "true");
    return true;
}

namespace xml
{

void XMLFile::release()
{
    doc.clear();
    buffer.clear();
}

void XMLFile::create_root(const char* root_name, bool write_declaration)
{
    K_ASSERT(root == nullptr, "XML file already has a root");

    if(write_declaration)
    {
        xml_node<>* decl = doc.allocate_node(node_declaration);
        decl->append_attribute(doc.allocate_attribute("version", "1.0"));
        decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
        doc.append_node(decl);
    }

    root = doc.allocate_node(node_element, doc.allocate_string(root_name));
    doc.append_node(root);
}

xml_node<>* XMLFile::add_node(xml_node<>* parent, const char* node_name, const char* node_value)
{
    xml_node<>* node = doc.allocate_node(node_element, doc.allocate_string(node_name));
    if(node_value)
        node->value(doc.allocate_string(node_value));
    parent->append_node(node);
    return node;
}

xml_attribute<>* XMLFile::add_attribute(xml_node<>* node, const char* attr_name, const char* attr_val)
{
    char* al_attr_name = doc.allocate_string(attr_name);
    char* al_attr_val = doc.allocate_string(attr_val);
    xml_attribute<>* attr = doc.allocate_attribute(al_attr_name, al_attr_val);
    node->append_attribute(attr);
    return attr;
}

void XMLFile::set_value(xml_node<>* node, const char* value) { node->value(doc.allocate_string(value)); }

bool XMLFile::read()
{
    KLOG("core", 0) << "Parsing XML file:" << std::endl;
    KLOGI << kb::WCC('p') << filepath << std::endl;

    if(!filepath.exists())
    {
        KLOGE("core") << "File does not exist." << std::endl;
        return false;
    }

    // Read the xml file into buffer
    buffer = wfs::get_file_as_string(filepath);

    // Parse the buffer using the xml file parsing library into doc
    try
    {
        doc.parse<0>(buffer.data());
        doc.validate();
    }
    catch(parse_error& e)
    {
        KLOGE("core") << "Parse error: " << e.what() << std::endl << "At: " << e.where<char>() << std::endl;
        return false;
    }
    catch(validation_error& e)
    {
        KLOGE("core") << "Validation error: " << e.what() << std::endl;
        return false;
    }

    // Check that a root node exists
    root = doc.first_node();
    if(!root)
    {
        KLOGW("core") << "File has no root node." << std::endl;
        return false;
    }

    return true;
}

void XMLFile::write()
{
    auto ofs = wfs::get_ostream(filepath, wfs::ascii);
    (*ofs) << doc;
}

template <>
void parse_node<const char*>(xml_node<>* parent, const char* name, std::function<void(const char* value)> exec)
{
    xml_node<>* leaf_node = parent->first_node(name);
    if(!leaf_node)
        return;

    exec(leaf_node->value());
}

bool parse_attribute(xml_node<>* node, const char* name, std::string& destination)
{
    rapidxml::xml_attribute<>* pAttr = node->first_attribute(name);
    if(!pAttr)
        return false;

    destination = pAttr->value();
    return true;
}

hash_t parse_attribute_h(rapidxml::xml_node<>* node, const char* name)
{
    rapidxml::xml_attribute<>* pAttr = node->first_attribute(name);
    if(!pAttr)
        return 0;

    return H_(node->first_attribute(name)->value());
}

bool parse_node(xml_node<>* parent, const char* leaf_name, std::string& destination)
{
    xml_node<>* leaf_node = parent->first_node(leaf_name);
    if(!leaf_node)
        return false;

    destination = leaf_node->value();
    return true;
}

hash_t parse_node_h(rapidxml::xml_node<>* parent, const char* leaf_name)
{
    xml_node<>* leaf_node = parent->first_node(leaf_name);
    if(!leaf_node)
        return 0;

    return H_(leaf_node->value());
}

template <>
bool set_attribute(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node, const char* name,
                   const std::string& source)
{
    rapidxml::xml_attribute<>* pAttr = node->first_attribute(name);
    if(!pAttr)
        return false;

    char* attr_value = doc.allocate_string(source.c_str());
    pAttr->value(attr_value);
    return true;
}

} // namespace xml
} // namespace erwin
