#include "input/input.h"
#include "event/window_events.h"
#include "event/event_bus.h"
#include "filesystem/filesystem.h"
#include "filesystem/xml_file.h"
#include "debug/logger.h"

namespace erwin
{

static struct
{
	eastl::map<hash_t, Input::ActionDescriptor> actions;
	eastl::vector<eastl::vector<hash_t>> triggers;
	eastl::vector<hash_t> order;
} s_storage;

[[maybe_unused]] static bool on_keyboard_event(const KeyboardEvent& event)
{	
	const auto& triggers = s_storage.triggers[(int)event.key];
	if(triggers.empty())
		return false;

	for(hash_t trigger: triggers)
	{
		const auto& desc = s_storage.actions.at(trigger);
		if(event.pressed == desc.pressed && event.repeat == desc.repeat)
			EVENTBUS.publish(ActionTriggeredEvent(trigger));
	}
	return true;
}

void Input::init()
{
	s_storage.triggers.resize(keymap::KEY_NAMES.size());

	load_config();

	// Disconnected for now
	// EVENTBUS.subscribe(&on_keyboard_event);
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

		register_action(action_name, description, key, trigger);
    }

    return true;
}

void Input::register_action(const std::string& action, const std::string& description, keymap::WKEY key, bool pressed)
{
	// TODO: detect possible conflicts
	hash_t hname = H_(action.c_str());
	s_storage.actions.emplace(hname, ActionDescriptor{key, pressed, false, action, description});
	s_storage.order.push_back(hname);
}

void Input::modify_action(hash_t action, keymap::WKEY key)
{
	auto it = s_storage.actions.find(action);
	W_ASSERT(it!=s_storage.actions.end(), "Unknown action");
	it->second.key = key;
	// TODO: also modify triggers
}

void Input::trigger_action_event(hash_t action)
{
	W_ASSERT(false, "Feature disabled.");
	auto it = s_storage.actions.find(action);
	W_ASSERT(it!=s_storage.actions.end(), "Unknown action");
	s_storage.triggers[(int)it->second.key].push_back(action);
}

bool Input::is_action_key_pressed(hash_t action)
{
	auto it = s_storage.actions.find(action);
	W_ASSERT(it!=s_storage.actions.end(), "Unknown action");
	return is_key_pressed(it->second.key);
}

keymap::WKEY Input::get_action_key(hash_t action)
{
	auto it = s_storage.actions.find(action);
	W_ASSERT(it!=s_storage.actions.end(), "Unknown action");
	return it->second.key;
}

uint32_t Input::get_action_count()
{
	return s_storage.order.size();
}

std::pair<hash_t, const Input::ActionDescriptor&> Input::get_action(uint32_t index)
{
	hash_t action = s_storage.order[index];
	return {action, s_storage.actions.at(action)};
}

bool Input::load_config()
{
	DLOGN("config") << "Loading keybindings." << std::endl;
	// * Check if a keybindings file exists in user directory
	// * Check if a default keybindings file exists in config directory
	// * If both default and user keybindings exist, load the most recent
	// * Else copy default to user and load user

	fs::path user_filepath    = filesystem::get_user_dir() / "config/keybindings.xml";
	bool has_user             = fs::exists(user_filepath);
	fs::path default_filepath = filesystem::get_config_dir() / "default_keybindings.xml";
	bool has_default          = fs::exists(default_filepath);

	if(!has_default && !has_user)
	{
		DLOGE("config") << "Failed to open user and default keybindings files." << std::endl;
		return false;
	}

	bool copy_default = (has_default && !has_user);
	if(has_default && has_user)
	{
		// Copy default if more recent
		auto ftime_u         = fs::last_write_time(user_filepath);
		std::time_t cftime_u = decltype(ftime_u)::clock::to_time_t(ftime_u);
		auto ftime_d         = fs::last_write_time(default_filepath);
		std::time_t cftime_d = decltype(ftime_d)::clock::to_time_t(ftime_d);
		copy_default = (cftime_d > cftime_u);
	}

	if(copy_default)
	{
		DLOGI << "Copying default." << std::endl;
		if(fs::exists(default_filepath))
			fs::copy_file(default_filepath, user_filepath, fs::copy_options::overwrite_existing);
	}

	// Read file and parse
	xml::XMLFile kbd_f(user_filepath);
	if(!kbd_f.read())
		return false;

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

	for(uint32_t ii=0; ii<s_storage.order.size(); ++ii)
	{
		auto&& [hname, action] = get_action(ii);

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