#pragma once

#include "core/core.h"

namespace erwin
{

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

	ACTION_COUNT,
};

[[maybe_unused]] static Action name_to_action(hash_t name)
{
	switch(name)
	{
		default: 								return ACTION_NONE;
		case "ACTION_FREEFLY_TOGGLE"_h: 		return ACTION_FREEFLY_TOGGLE;
		case "ACTION_FREEFLY_MOVE_FORWARD"_h: 	return ACTION_FREEFLY_MOVE_FORWARD;
		case "ACTION_FREEFLY_MOVE_BACKWARD"_h: 	return ACTION_FREEFLY_MOVE_BACKWARD;
		case "ACTION_FREEFLY_STRAFE_LEFT"_h: 	return ACTION_FREEFLY_STRAFE_LEFT;
		case "ACTION_FREEFLY_STRAFE_RIGHT"_h: 	return ACTION_FREEFLY_STRAFE_RIGHT;
		case "ACTION_FREEFLY_ASCEND"_h: 		return ACTION_FREEFLY_ASCEND;
		case "ACTION_FREEFLY_DESCEND"_h: 		return ACTION_FREEFLY_DESCEND;
		case "ACTION_FREEFLY_GO_FAST"_h: 		return ACTION_FREEFLY_GO_FAST;
	}
	return ACTION_NONE;
}

} // namespace erwin