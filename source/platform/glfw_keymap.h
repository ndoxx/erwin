#pragma once

#include "GLFW/glfw3.h"
#include "../Erwin/event/keys.h"

namespace erwin
{
namespace keymap
{

[[maybe_unused]] static WKEY GLFW_KEY_to_WKEY(uint16_t glfw_key)
{
    switch(glfw_key)
    {
        case(GLFW_KEY_SPACE): 		  return WKEY::SPACE;
        case(GLFW_KEY_APOSTROPHE): 	  return WKEY::APOSTROPHE;
        case(GLFW_KEY_COMMA): 		  return WKEY::COMMA;
        case(GLFW_KEY_MINUS): 		  return WKEY::MINUS;
        case(GLFW_KEY_PERIOD): 		  return WKEY::PERIOD;
        case(GLFW_KEY_SLASH): 		  return WKEY::SLASH;
        case(GLFW_KEY_0): 			  return WKEY::UB_0;
        case(GLFW_KEY_1): 			  return WKEY::UB_1;
        case(GLFW_KEY_2): 			  return WKEY::UB_2;
        case(GLFW_KEY_3): 			  return WKEY::UB_3;
        case(GLFW_KEY_4): 			  return WKEY::UB_4;
        case(GLFW_KEY_5): 			  return WKEY::UB_5;
        case(GLFW_KEY_6): 			  return WKEY::UB_6;
        case(GLFW_KEY_7): 			  return WKEY::UB_7;
        case(GLFW_KEY_8): 			  return WKEY::UB_8;
        case(GLFW_KEY_9): 			  return WKEY::UB_9;
        case(GLFW_KEY_SEMICOLON): 	  return WKEY::SEMICOLON;
        case(GLFW_KEY_EQUAL): 		  return WKEY::EQUAL;
        case(GLFW_KEY_A): 			  return WKEY::A;
        case(GLFW_KEY_B): 			  return WKEY::B;
        case(GLFW_KEY_C): 			  return WKEY::C;
        case(GLFW_KEY_D): 			  return WKEY::D;
        case(GLFW_KEY_E): 			  return WKEY::E;
        case(GLFW_KEY_F): 			  return WKEY::F;
        case(GLFW_KEY_G): 			  return WKEY::G;
        case(GLFW_KEY_H): 			  return WKEY::H;
        case(GLFW_KEY_I): 			  return WKEY::I;
        case(GLFW_KEY_J): 			  return WKEY::J;
        case(GLFW_KEY_K): 			  return WKEY::K;
        case(GLFW_KEY_L): 			  return WKEY::L;
        case(GLFW_KEY_M): 			  return WKEY::M;
        case(GLFW_KEY_N): 			  return WKEY::N;
        case(GLFW_KEY_O): 			  return WKEY::O;
        case(GLFW_KEY_P): 			  return WKEY::P;
        case(GLFW_KEY_Q): 			  return WKEY::Q;
        case(GLFW_KEY_R): 			  return WKEY::R;
        case(GLFW_KEY_S): 			  return WKEY::S;
        case(GLFW_KEY_T): 			  return WKEY::T;
        case(GLFW_KEY_U): 			  return WKEY::U;
        case(GLFW_KEY_V): 			  return WKEY::V;
        case(GLFW_KEY_W): 			  return WKEY::W;
        case(GLFW_KEY_X): 			  return WKEY::X;
        case(GLFW_KEY_Y): 			  return WKEY::Y;
        case(GLFW_KEY_Z): 			  return WKEY::Z;
        case(GLFW_KEY_LEFT_BRACKET):  return WKEY::LEFT_BRACKET;
        case(GLFW_KEY_BACKSLASH): 	  return WKEY::BACKSLASH;
        case(GLFW_KEY_RIGHT_BRACKET): return WKEY::RIGHT_BRACKET;
        case(GLFW_KEY_GRAVE_ACCENT):  return WKEY::GRAVE_ACCENT;
        case(GLFW_KEY_WORLD_1): 	  return WKEY::WORLD_1;
        case(GLFW_KEY_WORLD_2): 	  return WKEY::WORLD_2;
        case(GLFW_KEY_ESCAPE): 		  return WKEY::ESCAPE;
        case(GLFW_KEY_ENTER): 		  return WKEY::ENTER;
        case(GLFW_KEY_TAB): 		  return WKEY::TAB;
        case(GLFW_KEY_BACKSPACE): 	  return WKEY::BACKSPACE;
        case(GLFW_KEY_INSERT): 		  return WKEY::INSERT;
        case(GLFW_KEY_DELETE): 		  return WKEY::DELETE;
        case(GLFW_KEY_RIGHT): 		  return WKEY::RIGHT;
        case(GLFW_KEY_LEFT): 		  return WKEY::LEFT;
        case(GLFW_KEY_DOWN): 		  return WKEY::DOWN;
        case(GLFW_KEY_UP): 			  return WKEY::UP;
        case(GLFW_KEY_PAGE_UP): 	  return WKEY::PAGE_UP;
        case(GLFW_KEY_PAGE_DOWN): 	  return WKEY::PAGE_DOWN;
        case(GLFW_KEY_HOME): 		  return WKEY::HOME;
        case(GLFW_KEY_END): 		  return WKEY::END;
        case(GLFW_KEY_CAPS_LOCK): 	  return WKEY::CAPS_LOCK;
        case(GLFW_KEY_SCROLL_LOCK):   return WKEY::SCROLL_LOCK;
        case(GLFW_KEY_NUM_LOCK): 	  return WKEY::NUM_LOCK;
        case(GLFW_KEY_PRINT_SCREEN):  return WKEY::PRINT_SCREEN;
        case(GLFW_KEY_PAUSE): 		  return WKEY::PAUSE;
        case(GLFW_KEY_F1):   		  return WKEY::F1;
        case(GLFW_KEY_F2):   		  return WKEY::F2;
        case(GLFW_KEY_F3):   		  return WKEY::F3;
        case(GLFW_KEY_F4):   		  return WKEY::F4;
        case(GLFW_KEY_F5):   		  return WKEY::F5;
        case(GLFW_KEY_F6):   		  return WKEY::F6;
        case(GLFW_KEY_F7):   		  return WKEY::F7;
        case(GLFW_KEY_F8):   		  return WKEY::F8;
        case(GLFW_KEY_F9):   		  return WKEY::F9;
        case(GLFW_KEY_F10):  		  return WKEY::F10;
        case(GLFW_KEY_F11):  		  return WKEY::F11;
        case(GLFW_KEY_F12):  		  return WKEY::F12;
        case(GLFW_KEY_F13):  		  return WKEY::F13;
        case(GLFW_KEY_F14):  		  return WKEY::F14;
        case(GLFW_KEY_F15):  		  return WKEY::F15;
        case(GLFW_KEY_F16):  		  return WKEY::F16;
        case(GLFW_KEY_F17):  		  return WKEY::F17;
        case(GLFW_KEY_F18):  		  return WKEY::F18;
        case(GLFW_KEY_F19):  		  return WKEY::F19;
        case(GLFW_KEY_F20):  		  return WKEY::F20;
        case(GLFW_KEY_F21):  		  return WKEY::F21;
        case(GLFW_KEY_F22):  		  return WKEY::F22;
        case(GLFW_KEY_F23):  		  return WKEY::F23;
        case(GLFW_KEY_F24):  		  return WKEY::F24;
        case(GLFW_KEY_F25):  		  return WKEY::F25;
        case(GLFW_KEY_KP_0): 		  return WKEY::KP_0;
        case(GLFW_KEY_KP_1): 		  return WKEY::KP_1;
        case(GLFW_KEY_KP_2): 		  return WKEY::KP_2;
        case(GLFW_KEY_KP_3): 		  return WKEY::KP_3;
        case(GLFW_KEY_KP_4): 		  return WKEY::KP_4;
        case(GLFW_KEY_KP_5): 		  return WKEY::KP_5;
        case(GLFW_KEY_KP_6): 		  return WKEY::KP_6;
        case(GLFW_KEY_KP_7): 		  return WKEY::KP_7;
        case(GLFW_KEY_KP_8): 		  return WKEY::KP_8;
        case(GLFW_KEY_KP_9): 		  return WKEY::KP_9;
        case(GLFW_KEY_KP_DECIMAL):    return WKEY::KP_DECIMAL;
        case(GLFW_KEY_KP_DIVIDE):     return WKEY::KP_DIVIDE;
        case(GLFW_KEY_KP_MULTIPLY):   return WKEY::KP_MULTIPLY;
        case(GLFW_KEY_KP_SUBTRACT):   return WKEY::KP_SUBTRACT;
        case(GLFW_KEY_KP_ADD): 		  return WKEY::KP_ADD;
        case(GLFW_KEY_KP_ENTER): 	  return WKEY::KP_ENTER;
        case(GLFW_KEY_KP_EQUAL): 	  return WKEY::KP_EQUAL;
        case(GLFW_KEY_LEFT_SHIFT): 	  return WKEY::LEFT_SHIFT;
        case(GLFW_KEY_LEFT_CONTROL):  return WKEY::LEFT_CONTROL;
        case(GLFW_KEY_LEFT_ALT): 	  return WKEY::LEFT_ALT;
        case(GLFW_KEY_LEFT_SUPER): 	  return WKEY::LEFT_SUPER;
        case(GLFW_KEY_RIGHT_SHIFT):   return WKEY::RIGHT_SHIFT;
        case(GLFW_KEY_RIGHT_CONTROL): return WKEY::RIGHT_CONTROL;
        case(GLFW_KEY_RIGHT_ALT): 	  return WKEY::RIGHT_ALT;
        case(GLFW_KEY_RIGHT_SUPER):   return WKEY::RIGHT_SUPER;
        case(GLFW_KEY_MENU): 		  return WKEY::MENU; // Also GLFW_KEY_LAST
        default:                      return WKEY::NONE;
    }
};

[[maybe_unused]] static uint16_t WKEY_to_GLFW_KEY(WKEY wkey)
{
    switch(wkey)
    {
        case(WKEY::SPACE):         return GLFW_KEY_SPACE;
        case(WKEY::APOSTROPHE):    return GLFW_KEY_APOSTROPHE;
        case(WKEY::COMMA):         return GLFW_KEY_COMMA;
        case(WKEY::MINUS):         return GLFW_KEY_MINUS;
        case(WKEY::PERIOD):        return GLFW_KEY_PERIOD;
        case(WKEY::SLASH):         return GLFW_KEY_SLASH;
        case(WKEY::UB_0):          return GLFW_KEY_0;
        case(WKEY::UB_1):          return GLFW_KEY_1;
        case(WKEY::UB_2):          return GLFW_KEY_2;
        case(WKEY::UB_3):          return GLFW_KEY_3;
        case(WKEY::UB_4):          return GLFW_KEY_4;
        case(WKEY::UB_5):          return GLFW_KEY_5;
        case(WKEY::UB_6):          return GLFW_KEY_6;
        case(WKEY::UB_7):          return GLFW_KEY_7;
        case(WKEY::UB_8):          return GLFW_KEY_8;
        case(WKEY::UB_9):          return GLFW_KEY_9;
        case(WKEY::SEMICOLON):     return GLFW_KEY_SEMICOLON;
        case(WKEY::EQUAL):         return GLFW_KEY_EQUAL;
        case(WKEY::A):             return GLFW_KEY_A;
        case(WKEY::B):             return GLFW_KEY_B;
        case(WKEY::C):             return GLFW_KEY_C;
        case(WKEY::D):             return GLFW_KEY_D;
        case(WKEY::E):             return GLFW_KEY_E;
        case(WKEY::F):             return GLFW_KEY_F;
        case(WKEY::G):             return GLFW_KEY_G;
        case(WKEY::H):             return GLFW_KEY_H;
        case(WKEY::I):             return GLFW_KEY_I;
        case(WKEY::J):             return GLFW_KEY_J;
        case(WKEY::K):             return GLFW_KEY_K;
        case(WKEY::L):             return GLFW_KEY_L;
        case(WKEY::M):             return GLFW_KEY_M;
        case(WKEY::N):             return GLFW_KEY_N;
        case(WKEY::O):             return GLFW_KEY_O;
        case(WKEY::P):             return GLFW_KEY_P;
        case(WKEY::Q):             return GLFW_KEY_Q;
        case(WKEY::R):             return GLFW_KEY_R;
        case(WKEY::S):             return GLFW_KEY_S;
        case(WKEY::T):             return GLFW_KEY_T;
        case(WKEY::U):             return GLFW_KEY_U;
        case(WKEY::V):             return GLFW_KEY_V;
        case(WKEY::W):             return GLFW_KEY_W;
        case(WKEY::X):             return GLFW_KEY_X;
        case(WKEY::Y):             return GLFW_KEY_Y;
        case(WKEY::Z):             return GLFW_KEY_Z;
        case(WKEY::LEFT_BRACKET):  return GLFW_KEY_LEFT_BRACKET;
        case(WKEY::BACKSLASH):     return GLFW_KEY_BACKSLASH;
        case(WKEY::RIGHT_BRACKET): return GLFW_KEY_RIGHT_BRACKET;
        case(WKEY::GRAVE_ACCENT):  return GLFW_KEY_GRAVE_ACCENT;
        case(WKEY::WORLD_1):       return GLFW_KEY_WORLD_1;
        case(WKEY::WORLD_2):       return GLFW_KEY_WORLD_2;
        case(WKEY::ESCAPE):        return GLFW_KEY_ESCAPE;
        case(WKEY::ENTER):         return GLFW_KEY_ENTER;
        case(WKEY::TAB):           return GLFW_KEY_TAB;
        case(WKEY::BACKSPACE):     return GLFW_KEY_BACKSPACE;
        case(WKEY::INSERT):        return GLFW_KEY_INSERT;
        case(WKEY::DELETE):        return GLFW_KEY_DELETE;
        case(WKEY::RIGHT):         return GLFW_KEY_RIGHT;
        case(WKEY::LEFT):          return GLFW_KEY_LEFT;
        case(WKEY::DOWN):          return GLFW_KEY_DOWN;
        case(WKEY::UP):            return GLFW_KEY_UP;
        case(WKEY::PAGE_UP):       return GLFW_KEY_PAGE_UP;
        case(WKEY::PAGE_DOWN):     return GLFW_KEY_PAGE_DOWN;
        case(WKEY::HOME):          return GLFW_KEY_HOME;
        case(WKEY::END):           return GLFW_KEY_END;
        case(WKEY::CAPS_LOCK):     return GLFW_KEY_CAPS_LOCK;
        case(WKEY::SCROLL_LOCK):   return GLFW_KEY_SCROLL_LOCK;
        case(WKEY::NUM_LOCK):      return GLFW_KEY_NUM_LOCK;
        case(WKEY::PRINT_SCREEN):  return GLFW_KEY_PRINT_SCREEN;
        case(WKEY::PAUSE):         return GLFW_KEY_PAUSE;
        case(WKEY::F1):            return GLFW_KEY_F1;
        case(WKEY::F2):            return GLFW_KEY_F2;
        case(WKEY::F3):            return GLFW_KEY_F3;
        case(WKEY::F4):            return GLFW_KEY_F4;
        case(WKEY::F5):            return GLFW_KEY_F5;
        case(WKEY::F6):            return GLFW_KEY_F6;
        case(WKEY::F7):            return GLFW_KEY_F7;
        case(WKEY::F8):            return GLFW_KEY_F8;
        case(WKEY::F9):            return GLFW_KEY_F9;
        case(WKEY::F10):           return GLFW_KEY_F10;
        case(WKEY::F11):           return GLFW_KEY_F11;
        case(WKEY::F12):           return GLFW_KEY_F12;
        case(WKEY::F13):           return GLFW_KEY_F13;
        case(WKEY::F14):           return GLFW_KEY_F14;
        case(WKEY::F15):           return GLFW_KEY_F15;
        case(WKEY::F16):           return GLFW_KEY_F16;
        case(WKEY::F17):           return GLFW_KEY_F17;
        case(WKEY::F18):           return GLFW_KEY_F18;
        case(WKEY::F19):           return GLFW_KEY_F19;
        case(WKEY::F20):           return GLFW_KEY_F20;
        case(WKEY::F21):           return GLFW_KEY_F21;
        case(WKEY::F22):           return GLFW_KEY_F22;
        case(WKEY::F23):           return GLFW_KEY_F23;
        case(WKEY::F24):           return GLFW_KEY_F24;
        case(WKEY::F25):           return GLFW_KEY_F25;
        case(WKEY::KP_0):          return GLFW_KEY_KP_0;
        case(WKEY::KP_1):          return GLFW_KEY_KP_1;
        case(WKEY::KP_2):          return GLFW_KEY_KP_2;
        case(WKEY::KP_3):          return GLFW_KEY_KP_3;
        case(WKEY::KP_4):          return GLFW_KEY_KP_4;
        case(WKEY::KP_5):          return GLFW_KEY_KP_5;
        case(WKEY::KP_6):          return GLFW_KEY_KP_6;
        case(WKEY::KP_7):          return GLFW_KEY_KP_7;
        case(WKEY::KP_8):          return GLFW_KEY_KP_8;
        case(WKEY::KP_9):          return GLFW_KEY_KP_9;
        case(WKEY::KP_DECIMAL):    return GLFW_KEY_KP_DECIMAL;
        case(WKEY::KP_DIVIDE):     return GLFW_KEY_KP_DIVIDE;
        case(WKEY::KP_MULTIPLY):   return GLFW_KEY_KP_MULTIPLY;
        case(WKEY::KP_SUBTRACT):   return GLFW_KEY_KP_SUBTRACT;
        case(WKEY::KP_ADD):        return GLFW_KEY_KP_ADD;
        case(WKEY::KP_ENTER):      return GLFW_KEY_KP_ENTER;
        case(WKEY::KP_EQUAL):      return GLFW_KEY_KP_EQUAL;
        case(WKEY::LEFT_SHIFT):    return GLFW_KEY_LEFT_SHIFT;
        case(WKEY::LEFT_CONTROL):  return GLFW_KEY_LEFT_CONTROL;
        case(WKEY::LEFT_ALT):      return GLFW_KEY_LEFT_ALT;
        case(WKEY::LEFT_SUPER):    return GLFW_KEY_LEFT_SUPER;
        case(WKEY::RIGHT_SHIFT):   return GLFW_KEY_RIGHT_SHIFT;
        case(WKEY::RIGHT_CONTROL): return GLFW_KEY_RIGHT_CONTROL;
        case(WKEY::RIGHT_ALT):     return GLFW_KEY_RIGHT_ALT;
        case(WKEY::RIGHT_SUPER):   return GLFW_KEY_RIGHT_SUPER;
        case(WKEY::MENU):          return GLFW_KEY_MENU;
        case(WKEY::LAST):          return GLFW_KEY_LAST;
        default:                   return 0;
    }
};

[[maybe_unused]] static WMOUSE GLFW_MB_to_WMOUSE(uint16_t mb)
{
    switch(mb)
    {
    	case(GLFW_MOUSE_BUTTON_1): return WMOUSE::BUTTON_0; // Also GLFW_MOUSE_BUTTON_LEFT
    	case(GLFW_MOUSE_BUTTON_2): return WMOUSE::BUTTON_1; // Also GLFW_MOUSE_BUTTON_RIGHT
    	case(GLFW_MOUSE_BUTTON_3): return WMOUSE::BUTTON_2; // Also GLFW_MOUSE_BUTTON_MIDDLE
    	case(GLFW_MOUSE_BUTTON_4): return WMOUSE::BUTTON_3;
    	case(GLFW_MOUSE_BUTTON_5): return WMOUSE::BUTTON_4;
    	case(GLFW_MOUSE_BUTTON_6): return WMOUSE::BUTTON_5;
    	case(GLFW_MOUSE_BUTTON_7): return WMOUSE::BUTTON_6; // Also GLFW_MOUSE_BUTTON_LAST
        case(GLFW_MOUSE_BUTTON_8): return WMOUSE::BUTTON_7;
    	default:   	               return WMOUSE::NONE;
    }
};

[[maybe_unused]] static uint16_t WMOUSE_to_GLFW_MB(WMOUSE wm)
{
    switch(wm)
    {
        case(WMOUSE::BUTTON_0): return GLFW_MOUSE_BUTTON_1;
        case(WMOUSE::BUTTON_1): return GLFW_MOUSE_BUTTON_2;
        case(WMOUSE::BUTTON_2): return GLFW_MOUSE_BUTTON_3;
        case(WMOUSE::BUTTON_3): return GLFW_MOUSE_BUTTON_4;
        case(WMOUSE::BUTTON_4): return GLFW_MOUSE_BUTTON_5;
        case(WMOUSE::BUTTON_5): return GLFW_MOUSE_BUTTON_6;
        case(WMOUSE::BUTTON_6): return GLFW_MOUSE_BUTTON_7;
        case(WMOUSE::BUTTON_7): return GLFW_MOUSE_BUTTON_8;
        case(WMOUSE::BUTTON_8): return GLFW_MOUSE_BUTTON_LAST;
        default:                return 0;
    }
};

} // namespace keymap
} // namespace erwin