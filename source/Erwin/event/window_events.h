#pragma once

#include "event.h"

namespace erwin
{

struct WindowCloseEvent: public WEvent
{
    virtual void print(std::ostream& stream) const override
    {
        stream << "(void)";
    }
    virtual const char* get_name() const override { return "WindowCloseEvent"; }
};

struct WindowResizeEvent: public WEvent
{
	WindowResizeEvent(int width, int height):
	width(width),
	height(height)
	{

	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "new size: " << width << "x" << height;
    }
    virtual const char* get_name() const override { return "WindowResizeEvent"; }

    int width;
    int height;
};

struct KeyPressedEvent: public WEvent
{
	KeyPressedEvent(int key, bool repeat):
	key(key),
	repeat(repeat)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "key: " << key << " repeat: " << repeat;
    }
    virtual const char* get_name() const override { return "KeyPressedEvent"; }

    int key;
    bool repeat;
};

struct KeyReleasedEvent: public WEvent
{
	KeyReleasedEvent(int key):
	key(key)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "key: " << key;
    }
    virtual const char* get_name() const override { return "KeyReleasedEvent"; }

    int key;
};

struct MouseButtonPressedEvent: public WEvent
{
	MouseButtonPressedEvent(int button, float x, float y):
	button(button), x(x), y(y)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "button: " << button << " @ (" << x << "," << y << ")";
    }
    virtual const char* get_name() const override { return "MouseButtonPressedEvent"; }

    int button;
    float x;
    float y;
};

struct MouseButtonReleasedEvent: public WEvent
{
	MouseButtonReleasedEvent(int button, float x, float y):
	button(button), x(x), y(y)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "button: " << button << " @ (" << x << "," << y << ")";
    }
    virtual const char* get_name() const override { return "MouseButtonReleasedEvent"; }

    int button;
    float x;
    float y;
};

struct MouseMovedEvent: public WEvent
{
	MouseMovedEvent(float x, float y):
	x(x), y(y)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "cursor " << " @ (" << x << "," << y << ")";
    }
    virtual const char* get_name() const override { return "MouseMovedEvent"; }

    float x;
    float y;
};

struct MouseScrollEvent: public WEvent
{
	MouseScrollEvent(float x_offset, float y_offset):
	x_offset(x_offset), y_offset(y_offset)
	{
		
	}

    virtual void print(std::ostream& stream) const override
    {
        stream << "(" << x_offset << "," << y_offset << ")";
    }
    virtual const char* get_name() const override { return "MouseScrollEvent"; }

    float x_offset;
    float y_offset;
};

} // namespace erwin