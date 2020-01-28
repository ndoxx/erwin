#include "input/input.h"
#include "filesystem/filesystem.h"
#include "filesystem/xml_file.h"
#include "debug/logger.h"
#include "EASTL/map.h"
#include "EASTL/vector.h"

namespace erwin
{

struct ActionDescriptor
{
	keymap::WKEY key;
	bool trigger; // on key pressed: true; on key released: false;
	std::string name;
};

static struct
{
	eastl::map<hash_t, ActionDescriptor> actions;
	eastl::vector<eastl::vector<hash_t>> key_to_actions;
} s_storage;


bool Input::parse_keybindings(void* node)
{
	rapidxml::xml_node<>* xnode = static_cast<rapidxml::xml_node<>*>(node);

    // For each siblings at this recursion level
    for(rapidxml::xml_node<>* cur_node=xnode->first_node("action");
        cur_node;
        cur_node=cur_node->next_sibling("action"))
    {
        std::string action_name;
        if(!xml::parse_attribute(cur_node, "name", action_name)) return false;
        hash_t hkey_name = xml::parse_attribute_h(cur_node, "key");
		if(hkey_name == 0) return false;
		keymap::WKEY key = keymap::key_name_to_key(hkey_name);
		if(key == keymap::WKEY::NONE) continue;

        hash_t htrigger = xml::parse_attribute_h(cur_node, "trigger");
        bool trigger = (htrigger == "press"_h || htrigger == 0);

		register_action(action_name, key, trigger);
    }

    return true;
}

void Input::register_action(const std::string& action, keymap::WKEY key, bool pressed)
{
	hash_t hname = H_(action.c_str());
	// DLOGW("config") << action << " " << keymap::KEY_NAMES.at(key) << " " << pressed << std::endl;
	s_storage.actions.emplace(hname, ActionDescriptor{key, pressed, action});
}

bool Input::load_config()
{
	DLOGN("config") << "Loading keybindings." << std::endl;
	// * Check if a keybindings file exists in user directory, if found, load
	// * Else: Check if a default keybindings file exists in config directory, if found, copy to user dir and load
	fs::path filepath = filesystem::get_user_dir() / "config/keybindings.xml";
	if(!fs::exists(filepath))
	{
		DLOGI << "Copying default." << std::endl;
		fs::path default_filepath = filesystem::get_config_dir() / "default_keybindings.xml";
		if(fs::exists(default_filepath))
			fs::copy_file(default_filepath, filepath);
		else
		{
			DLOGE("config") << "Failed to open default keybindings file." << std::endl;
			return false;
		}
	}

	// Read file and parse
	xml::XMLFile kbd_f(filepath);
	if(!kbd_f.read())
		return false;

	s_storage.key_to_actions.resize(keymap::KEY_NAMES.size());

	bool success = parse_keybindings(kbd_f.root);

	return success;
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

	for(auto&& [hname, action]: s_storage.actions)
	{
		ofs << "\t<action name=\"" << action.name
			<< "\" key=\"" << keymap::KEY_NAMES.at(action.key)
			<< "\" trigger=\"" << (action.trigger ? "press" : "release")
			<< "\"/>" << std::endl;
	}
	ofs << "</Keymap>" << std::endl;

	return true;
}


} // namespace erwin