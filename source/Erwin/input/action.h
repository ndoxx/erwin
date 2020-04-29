#pragma once
#include "core/core.h"
namespace erwin
{

// ### This file was generated by scripts/gen_actions.py ###

static const std::string s_default_keybindings_path = "config/default_keybindings.xml";

// A client-side action enum should initialize its first value with Action::ACTION_COUNT
enum Action: uint32_t
{
	ACTION_NONE = 0,
	ACTION_FREEFLY_TOGGLE,
	ACTION_FREEFLY_MOVE_FORWARD,
	ACTION_FREEFLY_MOVE_BACKWARD,
	ACTION_FREEFLY_STRAFE_LEFT,
	ACTION_FREEFLY_STRAFE_RIGHT,
	ACTION_FREEFLY_ASCEND,
	ACTION_FREEFLY_DESCEND,
	ACTION_FREEFLY_GO_FAST,
	ACTION_DROP_SELECTION,
	ACTION_EDITOR_CYCLE_MODE,
	ACTION_COUNT,
};

[[maybe_unused]] static Action name_to_action(hash_t name)
{
	switch(name)
	{
		default: return ACTION_NONE;
		case "ACTION_FREEFLY_TOGGLE"_h: return ACTION_FREEFLY_TOGGLE;
		case "ACTION_FREEFLY_MOVE_FORWARD"_h: return ACTION_FREEFLY_MOVE_FORWARD;
		case "ACTION_FREEFLY_MOVE_BACKWARD"_h: return ACTION_FREEFLY_MOVE_BACKWARD;
		case "ACTION_FREEFLY_STRAFE_LEFT"_h: return ACTION_FREEFLY_STRAFE_LEFT;
		case "ACTION_FREEFLY_STRAFE_RIGHT"_h: return ACTION_FREEFLY_STRAFE_RIGHT;
		case "ACTION_FREEFLY_ASCEND"_h: return ACTION_FREEFLY_ASCEND;
		case "ACTION_FREEFLY_DESCEND"_h: return ACTION_FREEFLY_DESCEND;
		case "ACTION_FREEFLY_GO_FAST"_h: return ACTION_FREEFLY_GO_FAST;
		case "ACTION_DROP_SELECTION"_h: return ACTION_DROP_SELECTION;
		case "ACTION_EDITOR_CYCLE_MODE"_h: return ACTION_EDITOR_CYCLE_MODE;
	}
	return ACTION_NONE;
}
} // namespace erwin
