#pragma once

#include "event/event.h"
#include "input/keys.h"

namespace erwin
{

struct WindowCloseEvent: public WEvent
{
	EVENT_NAME(WindowCloseEvent)

    virtual void print(std::ostream& stream) const override
    {
        stream << "(void)";
    }
};

struct WindowResizeEvent: public WEvent
{
	EVENT_NAME(WindowResizeEvent)

	WindowResizeEvent(int width, int height):
	width(width),
	height(height)
	{

	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "new size: " << width << "x" << height;
    }

    int width;
    int height;
};

struct KeyboardEvent: public WEvent
{
	EVENT_NAME(KeyboardEvent)

	KeyboardEvent(keymap::WKEY key, uint8_t mods, bool pressed, bool repeat):
	key(key),
	mods(mods),
    pressed(pressed),
	repeat(repeat)
	{
		
	}

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

    keymap::WKEY key;
    uint8_t mods;
    bool pressed;
    bool repeat;
};

struct KeyTypedEvent: public WEvent
{
    EVENT_NAME(KeyTypedEvent)

    KeyTypedEvent(unsigned int codepoint):
    codepoint(codepoint)
    {

    }

    virtual void print(std::ostream& stream) const override
    {
        stream << codepoint;
    }

    unsigned int codepoint;
};

struct MouseButtonEvent: public WEvent
{
	EVENT_NAME(MouseButtonEvent)

	MouseButtonEvent(keymap::WMOUSE button, uint8_t mods, bool pressed, float x, float y):
	button(button), mods(mods), pressed(pressed), x(x), y(y)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << (pressed ? "PRE " : "REL ");
        stream << keymap::modifier_string(mods) << keymap::MB_NAMES.at(button) << " @ (" << x << "," << y << ")";
    }

    keymap::WMOUSE button;
    uint8_t mods;
    bool pressed;
    float x;
    float y;
};

struct MouseMovedEvent: public WEvent
{
	EVENT_NAME(MouseMovedEvent)

	MouseMovedEvent(float x, float y):
	x(x), y(y)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "cursor " << " @ (" << x << "," << y << ")";
    }

    float x;
    float y;
};

struct MouseScrollEvent: public WEvent
{
	EVENT_NAME(MouseScrollEvent)
	
	MouseScrollEvent(float x_offset, float y_offset):
	x_offset(x_offset), y_offset(y_offset)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "(" << x_offset << "," << y_offset << ")";
    }

    float x_offset;
    float y_offset;
};

} // namespace erwin