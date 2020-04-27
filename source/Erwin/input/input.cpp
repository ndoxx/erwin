#include "input/input.h"
#include "event/window_events.h"
#include "event/event_bus.h"
#include "filesystem/filesystem.h"
#include "filesystem/xml_file.h"
#include "debug/logger.h"

namespace erwin
{

#ifdef W_USE_EASTL
	eastl::vector<Input::ActionDescriptor> Input::actions;
#else
	std::vector<Input::ActionDescriptor> Input::actions;
#endif


void Input::init()
{
	// Push null action
	actions.push_back({keymap::WKEY::NONE, false, false, "", ""});

	load_config();
}

bool Input::parse_keybindings(void* node)
{
	rapidxml::xml_node<>* xnode = static_cast<rapidxml::xml_node<>*>(node);

    // For each siblings at this recursion level
    for(rapidxml::xml_node<>* cur_node=xnode->first_node("action");
        cur_node;
        cur_node=cur_node->next_sibling("action"))
    {
        std::string action_name, description;
        if(!xml::parse_attribute(cur_node, "name", action_name)) return false;
        if(!xml::parse_attribute(cur_node, "desc", description))
        	description = action_name;

        hash_t hkey_name = xml::parse_attribute_h(cur_node, "key");
		if(hkey_name == 0) return false;
		keymap::WKEY key = keymap::key_name_to_key(hkey_name);
		if(key == keymap::WKEY::NONE) continue;

        hash_t htrigger = xml::parse_attribute_h(cur_node, "trigger");
        bool trigger = (htrigger == "press"_h || htrigger == 0);

		// The XML file we're parsing better be in the correct enum order!
		actions.push_back({key, trigger, false, action_name, description});
    }

    return true;
}

bool Input::load_config()
{
	DLOGN("config") << "Loading keybindings." << std::endl;

	fs::path user_filepath    = filesystem::get_user_dir() / "config/keybindings.xml";
	fs::path default_filepath = filesystem::get_root_dir() / s_default_keybindings_path;
	if(!filesystem::ensure_user_config(user_filepath, default_filepath))
		return false;

	// Read file and parse
	xml::XMLFile kbd_f(user_filepath);
	if(!kbd_f.read())
		return false;

	return parse_keybindings(kbd_f.root);
}

bool Input::save_config()
{
	fs::path filepath = filesystem::get_user_dir() / "config/keybindings.xml";
	DLOG("config",1) << "Saving key bindings:" << std::endl;
	DLOGI << WCC('p') << filepath << std::endl;
	// Direct XML output for now
	std::ofstream ofs(filepath);
	ofs << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
	ofs << "<Keymap>" << std::endl;

	for(uint32_t ii=1; ii<actions.size(); ++ii)
	{
		const auto& action = actions[ii];
		ofs << "\t<action name=\"" << action.name
			<< "\" desc=\"" << action.description
			<< "\" key=\"" << keymap::KEY_NAMES.at(action.key)
			<< "\" trigger=\"" << (action.pressed ? "press" : "release")
			<< "\"/>" << std::endl;
	}
	ofs << "</Keymap>" << std::endl;

	return true;
}


} // namespace erwin