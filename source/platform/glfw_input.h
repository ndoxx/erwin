#pragma once

#include "input/input.h"

namespace erwin
{

class GLFWInput: public Input
{
protected:
	virtual bool is_key_pressed_impl(keymap::WKEY keycode) const override;
	virtual bool is_mouse_button_pressed_impl(keymap::WMOUSE button) const override;
	virtual std::pair<float,float> get_mouse_position_impl() const override;

private:
};


} // namespace erwin