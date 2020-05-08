#pragma once

#include <ostream>
#include "input/keys.h"
#include "event/event.h"
#include "core/core.h"
#include "glm/glm.hpp"

namespace erwin
{

struct WindowCloseEvent
{
    EVENT_DECLARATIONS(WindowCloseEvent);

    WindowCloseEvent() = default;
    
#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const WindowCloseEvent&)
    {
        stream << "(void)";
        return stream;
    }
#endif
};

struct WindowResizeEvent
{
    EVENT_DECLARATIONS(WindowResizeEvent);

    WindowResizeEvent() = default;
	WindowResizeEvent(int width, int height):
	width(width),
	height(height)
	{

	}

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const WindowResizeEvent& rhs)
    {
        stream << "new size: " << rhs.width << "x" << rhs.height;
        return stream;
    }
#endif

    int width;
    int height;
};

struct WindowMovedEvent
{
    EVENT_DECLARATIONS(WindowMovedEvent);

    WindowMovedEvent() = default;
    WindowMovedEvent(int xx, int yy):
    x(xx),
    y(yy)
    {

    }

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const WindowMovedEvent& rhs)
    {
        stream << "new position: (" << rhs.x << "," << rhs.y << ")";
        return stream;
    }
#endif

    int x;
    int y;
};

struct FramebufferResizeEvent
{
    EVENT_DECLARATIONS(FramebufferResizeEvent);

    FramebufferResizeEvent() = default;
    FramebufferResizeEvent(int width, int height):
    width(width),
    height(height)
    {

    }

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const FramebufferResizeEvent& rhs)
    {
        stream << "new size: " << rhs.width << "x" << rhs.height;
        return stream;
    }
#endif

    int width;
    int height;
};

struct BeginFrameEvent
{
    EVENT_DECLARATIONS(BeginFrameEvent);

    BeginFrameEvent() = default;
    
#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const BeginFrameEvent&)
    {
        stream << "(void)";
        return stream;
    }
#endif
};

struct KeyboardEvent
{
    EVENT_DECLARATIONS(KeyboardEvent);

    KeyboardEvent() = default;
	KeyboardEvent(keymap::WKEY key, uint8_t mods, bool pressed, bool repeat):
	key(key),
	mods(mods),
    pressed(pressed),
	repeat(repeat)
	{
		
	}

    inline bool mod_shift() const   { return mods & keymap::WKEYMOD::SHIFT; }
    inline bool mod_control() const { return mods & keymap::WKEYMOD::CONTROL; }
    inline bool mod_alt() const     { return mods & keymap::WKEYMOD::ALT; }

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const KeyboardEvent& rhs)
    {
        stream << (rhs.pressed ? "PRE " : "REL ");
    	if(is_modifier_key(rhs.key))
    	{
        	stream << keymap::KEY_NAMES.at(rhs.key) << (rhs.repeat ? " (r)" : "");
        }
        else
        {
        	stream << keymap::modifier_string(rhs.mods) << keymap::KEY_NAMES.at(rhs.key)
                   << (rhs.repeat ? " (r)" : "");
        }
        return stream;
    }
#endif

    keymap::WKEY key;
    uint8_t mods;
    bool pressed;
    bool repeat;
};

struct KeyTypedEvent
{
    EVENT_DECLARATIONS(KeyTypedEvent);

    KeyTypedEvent() = default;
    explicit KeyTypedEvent(unsigned int codepoint):
    codepoint(codepoint)
    {

    }

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const KeyTypedEvent& rhs)
    {
        stream << rhs.codepoint;
        return stream;
    }
#endif

    unsigned int codepoint;
};

struct MouseButtonEvent
{
    EVENT_DECLARATIONS(MouseButtonEvent);

    MouseButtonEvent() = default;
	MouseButtonEvent(keymap::WMOUSE button, uint8_t mods, bool pressed, float x, float y):
	button(button), mods(mods), pressed(pressed), x(x), y(y)
	{
		
	}

    inline bool mod_shift() const   { return mods & keymap::WKEYMOD::SHIFT; }
    inline bool mod_control() const { return mods & keymap::WKEYMOD::CONTROL; }
    inline bool mod_alt() const     { return mods & keymap::WKEYMOD::ALT; }

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const MouseButtonEvent& rhs)
    {
        stream << (rhs.pressed ? "PRE " : "REL ");
        stream << keymap::modifier_string(rhs.mods) << keymap::MB_NAMES.at(rhs.button) << " @ (" << rhs.x << "," << rhs.y << ")";
        return stream;
    }
#endif

    keymap::WMOUSE button;
    uint8_t mods;
    bool pressed;
    float x;
    float y;
};

struct MouseMovedEvent
{
    EVENT_DECLARATIONS(MouseMovedEvent);

    MouseMovedEvent() = default;
	MouseMovedEvent(float x, float y):
	x(x), y(y)
	{
		
	}

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const MouseMovedEvent& rhs)
    {
        stream << "cursor " << " @ (" << rhs.x << "," << rhs.y << ")";
        return stream;
    }
#endif

    float x;
    float y;
};

struct MouseScrollEvent
{
    EVENT_DECLARATIONS(MouseScrollEvent);

    MouseScrollEvent() = default;
	MouseScrollEvent(float x_offset, float y_offset):
	x_offset(x_offset), y_offset(y_offset)
	{
		
	}

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const MouseScrollEvent& rhs)
    {
        stream << "(" << rhs.x_offset << "," << rhs.y_offset << ")";
        return stream;
    }
#endif

    float x_offset;
    float y_offset;
};

struct RaySceneQueryEvent
{
    EVENT_DECLARATIONS(RaySceneQueryEvent);

    RaySceneQueryEvent() = default;
    explicit RaySceneQueryEvent(const glm::vec2& screen_coords):
    coords(screen_coords)
    {

    }

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const RaySceneQueryEvent& rhs)
    {
        stream << "(" << rhs.coords.x << "," << rhs.coords.y << ")";
        return stream;
    }
#endif

    glm::vec2 coords;
};

struct ActionTriggeredEvent
{
    EVENT_DECLARATIONS(ActionTriggeredEvent);

    ActionTriggeredEvent() = default;
    explicit ActionTriggeredEvent(hash_t action_name):
    action(action_name)
    {

    }

#ifdef W_DEBUG
    friend std::ostream& operator <<(std::ostream& stream, const ActionTriggeredEvent& rhs)
    {
        stream << rhs.action;
        return stream;
    }
#endif

    hash_t action;
};

} // namespace erwin