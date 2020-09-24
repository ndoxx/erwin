#pragma once

#ifdef W_USE_EASTL
	#include "EASTL/map.h"
#else
	#include <map>
#endif

#include "core/core.h"

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

[[maybe_unused]] static WKEYMOD mod_name_to_mod(hash_t mod_name)
{
	switch(mod_name)
	{
		default:		  return WKEYMOD::NONE;
		case "SHIFT"_h:   return WKEYMOD::SHIFT;
		case "CONTROL"_h: return WKEYMOD::CONTROL;
		case "CTRL"_h:    return WKEYMOD::CONTROL;
		case "ALT"_h:     return WKEYMOD::ALT;
	}
}

using KeyNameMap = std::map<WKEY, std::string>;

static const KeyNameMap KEY_NAMES =
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


[[maybe_unused]] static WKEY key_name_to_key(hash_t key_name)
{
	switch(key_name)
	{
		default:		  			return WKEY::NONE;
		case "<NONE>"_h:  			return WKEY::NONE;
		case "SPACE"_h:  			return WKEY::SPACE;
		case "APOSTROPHE"_h:  		return WKEY::APOSTROPHE;
		case "COMMA"_h:  			return WKEY::COMMA;
		case "MINUS"_h:  			return WKEY::MINUS;
		case "PERIOD"_h:  			return WKEY::PERIOD;
		case "SLASH"_h:  			return WKEY::SLASH;
		case "0"_h: 				return WKEY::UB_0;
		case "1"_h: 				return WKEY::UB_1;
		case "2"_h: 				return WKEY::UB_2;
		case "3"_h: 				return WKEY::UB_3;
		case "4"_h: 				return WKEY::UB_4;
		case "5"_h: 				return WKEY::UB_5;
		case "6"_h: 				return WKEY::UB_6;
		case "7"_h: 				return WKEY::UB_7;
		case "8"_h: 				return WKEY::UB_8;
		case "9"_h: 				return WKEY::UB_9;
		case "SEMICOLON"_h:  		return WKEY::SEMICOLON;
		case "EQUAL"_h:  			return WKEY::EQUAL;
		case "A"_h:  				return WKEY::A;
		case "B"_h:  				return WKEY::B;
		case "C"_h:  				return WKEY::C;
		case "D"_h:  				return WKEY::D;
		case "E"_h:  				return WKEY::E;
		case "F"_h:  				return WKEY::F;
		case "G"_h:  				return WKEY::G;
		case "H"_h:  				return WKEY::H;
		case "I"_h:  				return WKEY::I;
		case "J"_h:  				return WKEY::J;
		case "K"_h:  				return WKEY::K;
		case "L"_h:  				return WKEY::L;
		case "M"_h:  				return WKEY::M;
		case "N"_h:  				return WKEY::N;
		case "O"_h:  				return WKEY::O;
		case "P"_h:  				return WKEY::P;
		case "Q"_h:  				return WKEY::Q;
		case "R"_h:  				return WKEY::R;
		case "S"_h:  				return WKEY::S;
		case "T"_h:  				return WKEY::T;
		case "U"_h:  				return WKEY::U;
		case "V"_h:  				return WKEY::V;
		case "W"_h:  				return WKEY::W;
		case "X"_h:  				return WKEY::X;
		case "Y"_h:  				return WKEY::Y;
		case "Z"_h:  				return WKEY::Z;
		case "LEFT_BRACKET"_h:  	return WKEY::LEFT_BRACKET;
		case "BACKSLASH"_h:  		return WKEY::BACKSLASH;
		case "RIGHT_BRACKET"_h:  	return WKEY::RIGHT_BRACKET;
		case "GRAVE_ACCENT"_h:  	return WKEY::GRAVE_ACCENT;
		case "WORLD_1"_h:  			return WKEY::WORLD_1;
		case "WORLD_2"_h:  			return WKEY::WORLD_2;
		case "ESCAPE"_h:  			return WKEY::ESCAPE;
		case "ENTER"_h:  			return WKEY::ENTER;
		case "TAB"_h:  				return WKEY::TAB;
		case "BACKSPACE"_h:  		return WKEY::BACKSPACE;
		case "INSERT"_h:  			return WKEY::INSERT;
		case "DELETE"_h:  			return WKEY::DELETE;
		case "RIGHT"_h:  			return WKEY::RIGHT;
		case "LEFT"_h:  			return WKEY::LEFT;
		case "DOWN"_h:  			return WKEY::DOWN;
		case "UP"_h:  				return WKEY::UP;
		case "PAGE_UP"_h:  			return WKEY::PAGE_UP;
		case "PAGE_DOWN"_h:  		return WKEY::PAGE_DOWN;
		case "HOME"_h:  			return WKEY::HOME;
		case "END"_h:  				return WKEY::END;
		case "CAPS_LOCK"_h:  		return WKEY::CAPS_LOCK;
		case "SCROLL_LOCK"_h:  		return WKEY::SCROLL_LOCK;
		case "NUM_LOCK"_h:  		return WKEY::NUM_LOCK;
		case "PRINT_SCREEN"_h:  	return WKEY::PRINT_SCREEN;
		case "PAUSE"_h:  			return WKEY::PAUSE;
		case "F1"_h:  				return WKEY::F1;
		case "F2"_h:  				return WKEY::F2;
		case "F3"_h:  				return WKEY::F3;
		case "F4"_h:  				return WKEY::F4;
		case "F5"_h:  				return WKEY::F5;
		case "F6"_h:  				return WKEY::F6;
		case "F7"_h:  				return WKEY::F7;
		case "F8"_h:  				return WKEY::F8;
		case "F9"_h:  				return WKEY::F9;
		case "F10"_h:  				return WKEY::F10;
		case "F11"_h:  				return WKEY::F11;
		case "F12"_h:  				return WKEY::F12;
		case "F13"_h:  				return WKEY::F13;
		case "F14"_h:  				return WKEY::F14;
		case "F15"_h:  				return WKEY::F15;
		case "F16"_h:  				return WKEY::F16;
		case "F17"_h:  				return WKEY::F17;
		case "F18"_h:  				return WKEY::F18;
		case "F19"_h:  				return WKEY::F19;
		case "F20"_h:  				return WKEY::F20;
		case "F21"_h:  				return WKEY::F21;
		case "F22"_h:  				return WKEY::F22;
		case "F23"_h:  				return WKEY::F23;
		case "F24"_h:  				return WKEY::F24;
		case "F25"_h:  				return WKEY::F25;
		case "KP_0"_h:  			return WKEY::KP_0;
		case "KP_1"_h:  			return WKEY::KP_1;
		case "KP_2"_h:  			return WKEY::KP_2;
		case "KP_3"_h:  			return WKEY::KP_3;
		case "KP_4"_h:  			return WKEY::KP_4;
		case "KP_5"_h:  			return WKEY::KP_5;
		case "KP_6"_h:  			return WKEY::KP_6;
		case "KP_7"_h:  			return WKEY::KP_7;
		case "KP_8"_h:  			return WKEY::KP_8;
		case "KP_9"_h:  			return WKEY::KP_9;
		case "KP_DECIMAL"_h:  		return WKEY::KP_DECIMAL;
		case "KP_DIVIDE"_h:  		return WKEY::KP_DIVIDE;
		case "KP_MULTIPLY"_h:  		return WKEY::KP_MULTIPLY;
		case "KP_SUBTRACT"_h:  		return WKEY::KP_SUBTRACT;
		case "KP_ADD"_h:  			return WKEY::KP_ADD;
		case "KP_ENTER"_h:  		return WKEY::KP_ENTER;
		case "KP_EQUAL"_h:  		return WKEY::KP_EQUAL;
		case "LEFT_SHIFT"_h:  		return WKEY::LEFT_SHIFT;
		case "LEFT_CONTROL"_h:  	return WKEY::LEFT_CONTROL;
		case "LEFT_ALT"_h:  		return WKEY::LEFT_ALT;
		case "LEFT_SUPER"_h:  		return WKEY::LEFT_SUPER;
		case "RIGHT_SHIFT"_h:  		return WKEY::RIGHT_SHIFT;
		case "RIGHT_CONTROL"_h:  	return WKEY::RIGHT_CONTROL;
		case "RIGHT_ALT"_h:  		return WKEY::RIGHT_ALT;
		case "RIGHT_SUPER"_h:  		return WKEY::RIGHT_SUPER;
		case "MENU"_h:  			return WKEY::MENU;
		case "LAST"_h:  			return WKEY::LAST;
	}

	return WKEY::NONE;
}

using MouseButtonMap = std::map<WMOUSE, std::string>;

static const MouseButtonMap MB_NAMES =
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

[[maybe_unused]] static keymap::WKEYMOD key_to_keymod(WKEY key)
{
	switch(key)
	{
	    default:                          return keymap::WKEYMOD::NONE;
	    case keymap::WKEY::LEFT_CONTROL:  return keymap::WKEYMOD::CONTROL;
	    case keymap::WKEY::LEFT_SHIFT:    return keymap::WKEYMOD::SHIFT;
	    case keymap::WKEY::LEFT_ALT:      return keymap::WKEYMOD::ALT;
	    case keymap::WKEY::RIGHT_CONTROL: return keymap::WKEYMOD::CONTROL;
	    case keymap::WKEY::RIGHT_SHIFT:   return keymap::WKEYMOD::SHIFT;
	    case keymap::WKEY::RIGHT_ALT:     return keymap::WKEYMOD::ALT;
    }
}

[[maybe_unused]] static std::string modifier_string(uint8_t mods)
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