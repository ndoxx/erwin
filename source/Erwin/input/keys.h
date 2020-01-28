#pragma once

#include "EASTL/hash_map.h"

namespace erwin
{
namespace keymap
{

enum class WKEY: uint8_t
{
	NONE = 0,
	SPACE,
	APOSTROPHE,
	COMMA,
	MINUS,
	PERIOD,
	SLASH,
	UB_0,
	UB_1,
	UB_2,
	UB_3,
	UB_4,
	UB_5,
	UB_6,
	UB_7,
	UB_8,
	UB_9,
	SEMICOLON,
	EQUAL,
	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	LEFT_BRACKET,
	BACKSLASH,
	RIGHT_BRACKET,
	GRAVE_ACCENT,
	WORLD_1,
	WORLD_2,
	ESCAPE,
	ENTER,
	TAB,
	BACKSPACE,
	INSERT,
	DELETE,
	RIGHT,
	LEFT,
	DOWN,
	UP,
	PAGE_UP,
	PAGE_DOWN,
	HOME,
	END,
	CAPS_LOCK,
	SCROLL_LOCK,
	NUM_LOCK,
	PRINT_SCREEN,
	PAUSE,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
	F13,
	F14,
	F15,
	F16,
	F17,
	F18,
	F19,
	F20,
	F21,
	F22,
	F23,
	F24,
	F25,
	KP_0,
	KP_1,
	KP_2,
	KP_3,
	KP_4,
	KP_5,
	KP_6,
	KP_7,
	KP_8,
	KP_9,
	KP_DECIMAL,
	KP_DIVIDE,
	KP_MULTIPLY,
	KP_SUBTRACT,
	KP_ADD,
	KP_ENTER,
	KP_EQUAL,
	LEFT_SHIFT,
	LEFT_CONTROL,
	LEFT_ALT,
	LEFT_SUPER,
	RIGHT_SHIFT,
	RIGHT_CONTROL,
	RIGHT_ALT,
	RIGHT_SUPER,
	MENU,
	LAST,
};


enum class WMOUSE: uint8_t
{
	NONE = 0,
	BUTTON_0,
	BUTTON_1,
	BUTTON_2,
	BUTTON_3,
	BUTTON_4,
	BUTTON_5,
	BUTTON_6,
	BUTTON_7,
	BUTTON_8,
};

enum WKEYMOD: uint8_t
{
	NONE    = 0,
	SHIFT   = 1,
	CONTROL = 2,
	ALT     = 4
};

static const eastl::hash_map<WKEY, std::string> KEY_NAMES =
{
	{WKEY::NONE, 			"<NONE>"},
	{WKEY::SPACE, 			"SPACE"},
	{WKEY::APOSTROPHE, 		"APOSTROPHE"},
	{WKEY::COMMA, 			"COMMA"},
	{WKEY::MINUS, 			"MINUS"},
	{WKEY::PERIOD, 			"PERIOD"},
	{WKEY::SLASH, 			"SLASH"},
	{WKEY::UB_0,			"0"},
	{WKEY::UB_1,			"1"},
	{WKEY::UB_2,			"2"},
	{WKEY::UB_3,			"3"},
	{WKEY::UB_4,			"4"},
	{WKEY::UB_5,			"5"},
	{WKEY::UB_6,			"6"},
	{WKEY::UB_7,			"7"},
	{WKEY::UB_8,			"8"},
	{WKEY::UB_9,			"9"},
	{WKEY::SEMICOLON, 		"SEMICOLON"},
	{WKEY::EQUAL, 			"EQUAL"},
	{WKEY::A, 				"A"},
	{WKEY::B, 				"B"},
	{WKEY::C, 				"C"},
	{WKEY::D, 				"D"},
	{WKEY::E, 				"E"},
	{WKEY::F, 				"F"},
	{WKEY::G, 				"G"},
	{WKEY::H, 				"H"},
	{WKEY::I, 				"I"},
	{WKEY::J, 				"J"},
	{WKEY::K, 				"K"},
	{WKEY::L, 				"L"},
	{WKEY::M, 				"M"},
	{WKEY::N, 				"N"},
	{WKEY::O, 				"O"},
	{WKEY::P, 				"P"},
	{WKEY::Q, 				"Q"},
	{WKEY::R, 				"R"},
	{WKEY::S, 				"S"},
	{WKEY::T, 				"T"},
	{WKEY::U, 				"U"},
	{WKEY::V, 				"V"},
	{WKEY::W, 				"W"},
	{WKEY::X, 				"X"},
	{WKEY::Y, 				"Y"},
	{WKEY::Z, 				"Z"},
	{WKEY::LEFT_BRACKET, 	"LEFT_BRACKET"},
	{WKEY::BACKSLASH, 		"BACKSLASH"},
	{WKEY::RIGHT_BRACKET, 	"RIGHT_BRACKET"},
	{WKEY::GRAVE_ACCENT, 	"GRAVE_ACCENT"},
	{WKEY::WORLD_1, 		"WORLD_1"},
	{WKEY::WORLD_2, 		"WORLD_2"},
	{WKEY::ESCAPE, 			"ESCAPE"},
	{WKEY::ENTER, 			"ENTER"},
	{WKEY::TAB, 			"TAB"},
	{WKEY::BACKSPACE, 		"BACKSPACE"},
	{WKEY::INSERT, 			"INSERT"},
	{WKEY::DELETE, 			"DELETE"},
	{WKEY::RIGHT, 			"RIGHT"},
	{WKEY::LEFT, 			"LEFT"},
	{WKEY::DOWN, 			"DOWN"},
	{WKEY::UP, 				"UP"},
	{WKEY::PAGE_UP, 		"PAGE_UP"},
	{WKEY::PAGE_DOWN, 		"PAGE_DOWN"},
	{WKEY::HOME, 			"HOME"},
	{WKEY::END, 			"END"},
	{WKEY::CAPS_LOCK, 		"CAPS_LOCK"},
	{WKEY::SCROLL_LOCK, 	"SCROLL_LOCK"},
	{WKEY::NUM_LOCK, 		"NUM_LOCK"},
	{WKEY::PRINT_SCREEN, 	"PRINT_SCREEN"},
	{WKEY::PAUSE, 			"PAUSE"},
	{WKEY::F1, 				"F1"},
	{WKEY::F2, 				"F2"},
	{WKEY::F3, 				"F3"},
	{WKEY::F4, 				"F4"},
	{WKEY::F5, 				"F5"},
	{WKEY::F6, 				"F6"},
	{WKEY::F7, 				"F7"},
	{WKEY::F8, 				"F8"},
	{WKEY::F9, 				"F9"},
	{WKEY::F10, 			"F10"},
	{WKEY::F11, 			"F11"},
	{WKEY::F12, 			"F12"},
	{WKEY::F13, 			"F13"},
	{WKEY::F14, 			"F14"},
	{WKEY::F15, 			"F15"},
	{WKEY::F16, 			"F16"},
	{WKEY::F17, 			"F17"},
	{WKEY::F18, 			"F18"},
	{WKEY::F19, 			"F19"},
	{WKEY::F20, 			"F20"},
	{WKEY::F21, 			"F21"},
	{WKEY::F22, 			"F22"},
	{WKEY::F23, 			"F23"},
	{WKEY::F24, 			"F24"},
	{WKEY::F25, 			"F25"},
	{WKEY::KP_0, 			"KP_0"},
	{WKEY::KP_1, 			"KP_1"},
	{WKEY::KP_2, 			"KP_2"},
	{WKEY::KP_3, 			"KP_3"},
	{WKEY::KP_4, 			"KP_4"},
	{WKEY::KP_5, 			"KP_5"},
	{WKEY::KP_6, 			"KP_6"},
	{WKEY::KP_7, 			"KP_7"},
	{WKEY::KP_8, 			"KP_8"},
	{WKEY::KP_9, 			"KP_9"},
	{WKEY::KP_DECIMAL, 		"KP_DECIMAL"},
	{WKEY::KP_DIVIDE, 		"KP_DIVIDE"},
	{WKEY::KP_MULTIPLY, 	"KP_MULTIPLY"},
	{WKEY::KP_SUBTRACT, 	"KP_SUBTRACT"},
	{WKEY::KP_ADD, 			"KP_ADD"},
	{WKEY::KP_ENTER, 		"KP_ENTER"},
	{WKEY::KP_EQUAL, 		"KP_EQUAL"},
	{WKEY::LEFT_SHIFT, 		"LEFT_SHIFT"},
	{WKEY::LEFT_CONTROL, 	"LEFT_CONTROL"},
	{WKEY::LEFT_ALT, 		"LEFT_ALT"},
	{WKEY::LEFT_SUPER, 		"LEFT_SUPER"},
	{WKEY::RIGHT_SHIFT, 	"RIGHT_SHIFT"},
	{WKEY::RIGHT_CONTROL, 	"RIGHT_CONTROL"},
	{WKEY::RIGHT_ALT, 		"RIGHT_ALT"},
	{WKEY::RIGHT_SUPER, 	"RIGHT_SUPER"},
	{WKEY::MENU, 			"MENU"},
	{WKEY::LAST, 			"LAST"},
};

static const eastl::hash_map<WMOUSE, std::string> MB_NAMES =
{
	{WMOUSE::NONE, 	   "<NONE>"},
	{WMOUSE::BUTTON_0, "LMB"},
	{WMOUSE::BUTTON_1, "RMB"},
	{WMOUSE::BUTTON_2, "MMB"},
	{WMOUSE::BUTTON_3, "BUTTON_3"},
	{WMOUSE::BUTTON_4, "BUTTON_4"},
	{WMOUSE::BUTTON_5, "BUTTON_5"},
	{WMOUSE::BUTTON_6, "BUTTON_6"},
	{WMOUSE::BUTTON_7, "BUTTON_7"},
	{WMOUSE::BUTTON_8, "BUTTON_8"},
};

[[maybe_unused]] static bool is_modifier_key(WKEY key)
{
	return (key == keymap::WKEY::LEFT_CONTROL  ||
            key == keymap::WKEY::LEFT_SHIFT    ||
            key == keymap::WKEY::LEFT_ALT      ||
            key == keymap::WKEY::RIGHT_CONTROL ||
            key == keymap::WKEY::RIGHT_SHIFT   ||
            key == keymap::WKEY::RIGHT_ALT);
}

[[maybe_unused]] static std::string modifier_string(int mods)
{
	if(mods==0)
		return "";

	std::string ret;

	if(mods & WKEYMOD::SHIFT)
		ret += "SHIFT";
	if(mods & WKEYMOD::CONTROL)
	{
		if(ret.size()) ret += "+";
		ret += "CTRL";
	}
	if(mods & WKEYMOD::ALT)
	{
		if(ret.size()) ret += "+";
		ret += "ALT";
	}
	if(ret.size()) ret += "+";

	return ret;
}

} // namespace keymap
} // namespace erwin