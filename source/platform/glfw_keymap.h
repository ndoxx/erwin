#pragma once

#include "GLFW/glfw3.h"
#include "../Erwin/event/keys.h"

namespace erwin
{
namespace keymap
{

static const std::unordered_map<uint16_t, WKEY> GLFW_KEY_TO_WKEY = 
{
    {GLFW_KEY_SPACE, 			WKEY::SPACE},
    {GLFW_KEY_APOSTROPHE, 		WKEY::APOSTROPHE},
    {GLFW_KEY_COMMA, 			WKEY::COMMA},
    {GLFW_KEY_MINUS, 			WKEY::MINUS},
    {GLFW_KEY_PERIOD, 			WKEY::PERIOD},
    {GLFW_KEY_SLASH, 			WKEY::SLASH},
    {GLFW_KEY_0, 				WKEY::UB_0},
    {GLFW_KEY_1, 				WKEY::UB_1},
    {GLFW_KEY_2, 				WKEY::UB_2},
    {GLFW_KEY_3, 				WKEY::UB_3},
    {GLFW_KEY_4, 				WKEY::UB_4},
    {GLFW_KEY_5, 				WKEY::UB_5},
    {GLFW_KEY_6, 				WKEY::UB_6},
    {GLFW_KEY_7, 				WKEY::UB_7},
    {GLFW_KEY_8, 				WKEY::UB_8},
    {GLFW_KEY_9, 				WKEY::UB_9},
    {GLFW_KEY_SEMICOLON, 		WKEY::SEMICOLON},
    {GLFW_KEY_EQUAL, 			WKEY::EQUAL},
    {GLFW_KEY_A, 				WKEY::A},
    {GLFW_KEY_B, 				WKEY::B},
    {GLFW_KEY_C, 				WKEY::C},
    {GLFW_KEY_D, 				WKEY::D},
    {GLFW_KEY_E, 				WKEY::E},
    {GLFW_KEY_F, 				WKEY::F},
    {GLFW_KEY_G, 				WKEY::G},
    {GLFW_KEY_H, 				WKEY::H},
    {GLFW_KEY_I, 				WKEY::I},
    {GLFW_KEY_J, 				WKEY::J},
    {GLFW_KEY_K, 				WKEY::K},
    {GLFW_KEY_L, 				WKEY::L},
    {GLFW_KEY_M, 				WKEY::M},
    {GLFW_KEY_N, 				WKEY::N},
    {GLFW_KEY_O, 				WKEY::O},
    {GLFW_KEY_P, 				WKEY::P},
    {GLFW_KEY_Q, 				WKEY::Q},
    {GLFW_KEY_R, 				WKEY::R},
    {GLFW_KEY_S, 				WKEY::S},
    {GLFW_KEY_T, 				WKEY::T},
    {GLFW_KEY_U, 				WKEY::U},
    {GLFW_KEY_V, 				WKEY::V},
    {GLFW_KEY_W, 				WKEY::W},
    {GLFW_KEY_X, 				WKEY::X},
    {GLFW_KEY_Y, 				WKEY::Y},
    {GLFW_KEY_Z, 				WKEY::Z},
    {GLFW_KEY_LEFT_BRACKET, 	WKEY::LEFT_BRACKET},
    {GLFW_KEY_BACKSLASH, 		WKEY::BACKSLASH},
    {GLFW_KEY_RIGHT_BRACKET, 	WKEY::RIGHT_BRACKET},
    {GLFW_KEY_GRAVE_ACCENT, 	WKEY::GRAVE_ACCENT},
    {GLFW_KEY_WORLD_1, 			WKEY::WORLD_1},
    {GLFW_KEY_WORLD_2, 			WKEY::WORLD_2},
    {GLFW_KEY_ESCAPE, 			WKEY::ESCAPE},
    {GLFW_KEY_ENTER, 			WKEY::ENTER},
    {GLFW_KEY_TAB, 				WKEY::TAB},
    {GLFW_KEY_BACKSPACE, 		WKEY::BACKSPACE},
    {GLFW_KEY_INSERT, 			WKEY::INSERT},
    {GLFW_KEY_DELETE, 			WKEY::DELETE},
    {GLFW_KEY_RIGHT, 			WKEY::RIGHT},
    {GLFW_KEY_LEFT, 			WKEY::LEFT},
    {GLFW_KEY_DOWN, 			WKEY::DOWN},
    {GLFW_KEY_UP, 				WKEY::UP},
    {GLFW_KEY_PAGE_UP, 			WKEY::PAGE_UP},
    {GLFW_KEY_PAGE_DOWN, 		WKEY::PAGE_DOWN},
    {GLFW_KEY_HOME, 			WKEY::HOME},
    {GLFW_KEY_END, 				WKEY::END},
    {GLFW_KEY_CAPS_LOCK, 		WKEY::CAPS_LOCK},
    {GLFW_KEY_SCROLL_LOCK, 		WKEY::SCROLL_LOCK},
    {GLFW_KEY_NUM_LOCK, 		WKEY::NUM_LOCK},
    {GLFW_KEY_PRINT_SCREEN, 	WKEY::PRINT_SCREEN},
    {GLFW_KEY_PAUSE, 			WKEY::PAUSE},
    {GLFW_KEY_F1,   			WKEY::F1},
    {GLFW_KEY_F2,   			WKEY::F2},
    {GLFW_KEY_F3,   			WKEY::F3},
    {GLFW_KEY_F4,   			WKEY::F4},
    {GLFW_KEY_F5,   			WKEY::F5},
    {GLFW_KEY_F6,   			WKEY::F6},
    {GLFW_KEY_F7,   			WKEY::F7},
    {GLFW_KEY_F8,   			WKEY::F8},
    {GLFW_KEY_F9,   			WKEY::F9},
    {GLFW_KEY_F10,  			WKEY::F10},
    {GLFW_KEY_F11,  			WKEY::F11},
    {GLFW_KEY_F12,  			WKEY::F12},
    {GLFW_KEY_F13,  			WKEY::F13},
    {GLFW_KEY_F14,  			WKEY::F14},
    {GLFW_KEY_F15,  			WKEY::F15},
    {GLFW_KEY_F16,  			WKEY::F16},
    {GLFW_KEY_F17,  			WKEY::F17},
    {GLFW_KEY_F18,  			WKEY::F18},
    {GLFW_KEY_F19,  			WKEY::F19},
    {GLFW_KEY_F20,  			WKEY::F20},
    {GLFW_KEY_F21,  			WKEY::F21},
    {GLFW_KEY_F22,  			WKEY::F22},
    {GLFW_KEY_F23,  			WKEY::F23},
    {GLFW_KEY_F24,  			WKEY::F24},
    {GLFW_KEY_F25,  			WKEY::F25},
    {GLFW_KEY_KP_0, 			WKEY::KP_0},
    {GLFW_KEY_KP_1, 			WKEY::KP_1},
    {GLFW_KEY_KP_2, 			WKEY::KP_2},
    {GLFW_KEY_KP_3, 			WKEY::KP_3},
    {GLFW_KEY_KP_4, 			WKEY::KP_4},
    {GLFW_KEY_KP_5, 			WKEY::KP_5},
    {GLFW_KEY_KP_6, 			WKEY::KP_6},
    {GLFW_KEY_KP_7, 			WKEY::KP_7},
    {GLFW_KEY_KP_8, 			WKEY::KP_8},
    {GLFW_KEY_KP_9, 			WKEY::KP_9},
    {GLFW_KEY_KP_DECIMAL,  		WKEY::KP_DECIMAL},
    {GLFW_KEY_KP_DIVIDE,   		WKEY::KP_DIVIDE},
    {GLFW_KEY_KP_MULTIPLY, 		WKEY::KP_MULTIPLY},
    {GLFW_KEY_KP_SUBTRACT, 		WKEY::KP_SUBTRACT},
    {GLFW_KEY_KP_ADD, 			WKEY::KP_ADD},
    {GLFW_KEY_KP_ENTER, 		WKEY::KP_ENTER},
    {GLFW_KEY_KP_EQUAL, 		WKEY::KP_EQUAL},
    {GLFW_KEY_LEFT_SHIFT, 		WKEY::LEFT_SHIFT},
    {GLFW_KEY_LEFT_CONTROL, 	WKEY::LEFT_CONTROL},
    {GLFW_KEY_LEFT_ALT, 		WKEY::LEFT_ALT},
    {GLFW_KEY_LEFT_SUPER, 		WKEY::LEFT_SUPER},
    {GLFW_KEY_RIGHT_SHIFT, 		WKEY::RIGHT_SHIFT},
    {GLFW_KEY_RIGHT_CONTROL, 	WKEY::RIGHT_CONTROL},
    {GLFW_KEY_RIGHT_ALT, 		WKEY::RIGHT_ALT},
    {GLFW_KEY_RIGHT_SUPER, 		WKEY::RIGHT_SUPER},
    {GLFW_KEY_MENU, 			WKEY::MENU},
    {GLFW_KEY_LAST, 			WKEY::LAST},
};

static const std::unordered_map<uint16_t, WMOUSE> GLFW_MB_TO_WMOUSE = 
{
	{GLFW_MOUSE_BUTTON_1,   	WMOUSE::BUTTON_0},
	{GLFW_MOUSE_BUTTON_2,   	WMOUSE::BUTTON_1},
	{GLFW_MOUSE_BUTTON_3,   	WMOUSE::BUTTON_2},
	{GLFW_MOUSE_BUTTON_4,   	WMOUSE::BUTTON_3},
	{GLFW_MOUSE_BUTTON_5,   	WMOUSE::BUTTON_4},
	{GLFW_MOUSE_BUTTON_6,   	WMOUSE::BUTTON_5},
	{GLFW_MOUSE_BUTTON_7,   	WMOUSE::BUTTON_6},
	{GLFW_MOUSE_BUTTON_8,   	WMOUSE::BUTTON_7},
	{GLFW_MOUSE_BUTTON_LAST,   	WMOUSE::BUTTON_8},
	{GLFW_MOUSE_BUTTON_LEFT,   	WMOUSE::BUTTON_1},
	{GLFW_MOUSE_BUTTON_RIGHT,   WMOUSE::BUTTON_2},
	{GLFW_MOUSE_BUTTON_MIDDLE,  WMOUSE::BUTTON_3},
};

} // namespace keymap
} // namespace erwin