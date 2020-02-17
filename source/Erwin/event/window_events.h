#pragma once

#include "event/event.h"
#include "input/keys.h"
#include "glm/glm.hpp"

namespace erwin
{

struct WindowCloseEvent: public WEvent
{
	EVENT_DECLARATION(WindowCloseEvent);
    WindowCloseEvent() = default;
    
#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << "(void)";
    }
#endif
};

struct WindowResizeEvent: public WEvent
{
	EVENT_DECLARATION(WindowResizeEvent);

    WindowResizeEvent() = default;
	WindowResizeEvent(int width, int height):
	width(width),
	height(height)
	{

	}

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << "new size: " << width << "x" << height;
    }
#endif

    int width;
    int height;
};

struct WindowMovedEvent: public WEvent
{
    EVENT_DECLARATION(WindowMovedEvent);

    WindowMovedEvent() = default;
    WindowMovedEvent(int xx, int yy):
    x(xx),
    y(yy)
    {

    }

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << "new position: (" << x << "," << y << ")";
    }
#endif

    int x;
    int y;
};

struct FramebufferResizeEvent: public WEvent
{
    EVENT_DECLARATION(FramebufferResizeEvent);

    FramebufferResizeEvent() = default;
    FramebufferResizeEvent(int width, int height):
    width(width),
    height(height)
    {

    }

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << "new size: " << width << "x" << height;
    }
#endif

    int width;
    int height;
};

struct KeyboardEvent: public WEvent
{
	EVENT_DECLARATION(KeyboardEvent);

    KeyboardEvent() = default;
	KeyboardEvent(keymap::WKEY key, uint8_t mods, bool pressed, bool repeat):
	key(key),
	mods(mods),
    pressed(pressed),
	repeat(repeat)
	{
		
	}

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << (pressed ? "PRE " : "REL ");
    	if(is_modifier_key(key))
    	{
        	stream << keymap::KEY_NAMES.at(key) << (repeat ? " (r)" : "");
        }
        else
        {
        	stream << keymap::modifier_string(mods) << keymap::KEY_NAMES.at(key)
                   << (repeat ? " (r)" : "");
        }
    }
#endif

    keymap::WKEY key;
    uint8_t mods;
    bool pressed;
    bool repeat;
};

struct KeyTypedEvent: public WEvent
{
    EVENT_DECLARATION(KeyTypedEvent);

    KeyTypedEvent() = default;
    explicit KeyTypedEvent(unsigned int codepoint):
    codepoint(codepoint)
    {

    }

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << codepoint;
    }
#endif

    unsigned int codepoint;
};

struct MouseButtonEvent: public WEvent
{
	EVENT_DECLARATION(MouseButtonEvent);

    MouseButtonEvent() = default;
	MouseButtonEvent(keymap::WMOUSE button, uint8_t mods, bool pressed, float x, float y):
	button(button), mods(mods), pressed(pressed), x(x), y(y)
	{
		
	}

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << (pressed ? "PRE " : "REL ");
        stream << keymap::modifier_string(mods) << keymap::MB_NAMES.at(button) << " @ (" << x << "," << y << ")";
    }
#endif

    keymap::WMOUSE button;
    uint8_t mods;
    bool pressed;
    float x;
    float y;
};

struct MouseMovedEvent: public WEvent
{
	EVENT_DECLARATION(MouseMovedEvent);

    MouseMovedEvent() = default;
	MouseMovedEvent(float x, float y):
	x(x), y(y)
	{
		
	}

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << "cursor " << " @ (" << x << "," << y << ")";
    }
#endif

    float x;
    float y;
};

struct MouseScrollEvent: public WEvent
{
	EVENT_DECLARATION(MouseScrollEvent);
	
    MouseScrollEvent() = default;
	MouseScrollEvent(float x_offset, float y_offset):
	x_offset(x_offset), y_offset(y_offset)
	{
		
	}

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << "(" << x_offset << "," << y_offset << ")";
    }
#endif

    float x_offset;
    float y_offset;
};

struct RaySceneQueryEvent: public WEvent
{
    EVENT_DECLARATION(RaySceneQueryEvent);

    RaySceneQueryEvent() = default;
    explicit RaySceneQueryEvent(const glm::vec2& screen_coords):
    coords(screen_coords)
    {

    }

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << "(" << coords.x << "," << coords.y << ")";
    }
#endif

    glm::vec2 coords;
};

struct ActionTriggeredEvent: public WEvent
{
    EVENT_DECLARATION(ActionTriggeredEvent);

    ActionTriggeredEvent() = default;
    explicit ActionTriggeredEvent(hash_t action_name):
    action(action_name)
    {

    }

#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const override
    {
        stream << action;
    }
#endif

    hash_t action;
};

} // namespace erwin