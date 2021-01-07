#include "input/input.h"
#include "core/application.h"
#include "event/event_bus.h"
#include "event/window_events.h"
#include "filesystem/xml_file.h"
#include <kibble/logger/logger.h>
#include <kibble/string/string.h>

namespace erwin
{

std::vector<Input::ActionDescriptor> Input::actions;

void Input::init()
{
    // Push null action
    actions.push_back({keymap::WKEY::NONE, false, false, keymap::WKEYMOD::NONE, "", ""});

    load_config();
}

bool Input::parse_keybindings(void* node)
{
    rapidxml::xml_node<>* xnode = static_cast<rapidxml::xml_node<>*>(node);

    // For each siblings at this recursion level
    for(rapidxml::xml_node<>* cur_node = xnode->first_node("action"); cur_node;
        cur_node = cur_node->next_sibling("action"))
    {
        std::string action_name, description, key_comb;
        if(!xml::parse_attribute(cur_node, "name", action_name))
            return false;
        if(!xml::parse_attribute(cur_node, "desc", description))
            description = action_name;

        // Detect and extract key modifiers
        keymap::WKEY key = keymap::WKEY::NONE;
        uint8_t mods = keymap::WKEYMOD::NONE;
        if(!xml::parse_attribute(cur_node, "key", key_comb))
            return false;
        if(key_comb.find_first_of("+") != std::string::npos)
        {
            auto tokens = kb::su::tokenize(key_comb, '+');
            for(size_t ii = 0; ii < tokens.size() - 1; ++ii)
            {
                hash_t hmod = H_(tokens[ii].c_str());
                mods |= keymap::mod_name_to_mod(hmod);
            }
            hash_t hkey_name = H_(tokens.back().c_str());
            key = keymap::key_name_to_key(hkey_name);
        }
        else
        {
            hash_t hkey_name = H_(key_comb.c_str());
            key = keymap::key_name_to_key(hkey_name);
        }

        if(key == keymap::WKEY::NONE)
            continue;

        bool repeat = false;
        xml::parse_attribute(cur_node, "repeat", repeat);

        hash_t htrigger = xml::parse_attribute_h(cur_node, "trigger");
        bool trigger = (htrigger == "press"_h || htrigger == 0);

        // The XML file we're parsing better be in the correct enum order!
        actions.push_back({key, trigger, repeat, mods, action_name, description});
    }

    return true;
}

bool Input::load_config()
{
    KLOGN("config") << "Loading keybindings." << std::endl;

    auto user_filepath = "usr://keybindings.xml";
    std::string default_filepath(WFS().get_aliased_directory("root"_h) / s_default_keybindings_path);

    if(!APP().mirror_settings(user_filepath, default_filepath))
        return false;

    // Read file and parse
    xml::XMLFile kbd_f(user_filepath);
    if(!kbd_f.read())
        return false;

    return parse_keybindings(kbd_f.root);
}

bool Input::save_config()
{
    auto filepath = "usr://keybindings.xml";
    KLOG("config", 1) << "Saving key bindings:" << std::endl;
    KLOGI << kb::KS_PATH_ << filepath << std::endl;
    // Direct XML output for now
    std::ofstream ofs(WFS().regular_path(filepath), std::ios::binary);
    ofs << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
    ofs << "<Keymap>" << std::endl;

    for(uint32_t ii = 1; ii < actions.size(); ++ii)
    {
        const auto& action = actions[ii];
        std::string key_comb = keymap::modifier_string(action.mods) + keymap::KEY_NAMES.at(action.key);
        ofs << "\t<action name=\"" << action.name << "\" desc=\"" << action.description << "\" key=\"" << key_comb
            << "\" trigger=\"" << (action.pressed ? "press" : "release") << "\"/>" << std::endl;
    }
    ofs << "</Keymap>" << std::endl;

    return true;
}

} // namespace erwin