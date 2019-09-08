#pragma once

#include "event.h"
#include "keys.h"

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

struct KeyPressedEvent: public WEvent
{
	EVENT_NAME(KeyPressedEvent)

	KeyPressedEvent(keymap::WKEY key, int mods, bool repeat):
	key(key),
	mods(mods),
	repeat(repeat)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
    	if(key == keymap::WKEY::LEFT_CONTROL  ||
    	   key == keymap::WKEY::LEFT_SHIFT    ||
    	   key == keymap::WKEY::LEFT_ALT      ||
    	   key == keymap::WKEY::RIGHT_CONTROL ||
    	   key == keymap::WKEY::RIGHT_SHIFT   ||
    	   key == keymap::WKEY::RIGHT_ALT)
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
    int mods;
    bool repeat;
};

struct KeyReleasedEvent: public WEvent
{
	EVENT_NAME(KeyReleasedEvent)

	KeyReleasedEvent(keymap::WKEY key, int mods):
	key(key),
	mods(mods)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
    	if(key == keymap::WKEY::LEFT_CONTROL  ||
    	   key == keymap::WKEY::LEFT_SHIFT    ||
    	   key == keymap::WKEY::LEFT_ALT      ||
    	   key == keymap::WKEY::RIGHT_CONTROL ||
    	   key == keymap::WKEY::RIGHT_SHIFT   ||
    	   key == keymap::WKEY::RIGHT_ALT)
    	{
        	stream << keymap::KEY_NAMES.at(key);
        }
        else
        {
        	stream << keymap::modifier_string(mods) << keymap::KEY_NAMES.at(key);
        }
    }

    keymap::WKEY key;
    int mods;
};

struct MouseButtonPressedEvent: public WEvent
{
	EVENT_NAME(MouseButtonPressedEvent)

	MouseButtonPressedEvent(int button, float x, float y):
	button(button), x(x), y(y)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "button: " << button << " @ (" << x << "," << y << ")";
    }

    int button;
    float x;
    float y;
};

struct MouseButtonReleasedEvent: public WEvent
{
	EVENT_NAME(MouseButtonReleasedEvent)

	MouseButtonReleasedEvent(int button, float x, float y):
	button(button), x(x), y(y)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "button: " << button << " @ (" << x << "," << y << ")";
    }

    int button;
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