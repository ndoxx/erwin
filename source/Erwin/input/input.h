#pragma once

#include "core/core.h"
#include "input/keys.h"

namespace erwin
{

// Abstract class for input polling
class W_API Input
{
protected:
	Input() = default;
	virtual ~Input() = default;

public:
	friend class Application;

	NON_COPYABLE(Input);
	NON_MOVABLE(Input);

	// --- Action API ---
	static bool load_config();
	static bool save_config();
	static void register_action(const std::string& action, keymap::WKEY key, bool pressed);


	// --- Device interaction / polling --

	inline static bool is_key_pressed(keymap::WKEY keycode)
	{
		return INSTANCE_->is_key_pressed_impl(keycode);
	}
	inline static bool is_mouse_button_pressed(keymap::WMOUSE button)
	{
		return INSTANCE_->is_mouse_button_pressed_impl(button);
	}
	inline static std::pair<float,float> get_mouse_position()
	{
		return INSTANCE_->get_mouse_position_impl();
	}
	inline static void set_mouse_position(float x, float y)
	{
		INSTANCE_->set_mouse_position_impl(x,y);
	}
	inline static void center_mouse_position()
	{
		INSTANCE_->center_mouse_position_impl();
	}
	inline static void show_cursor(bool value)
	{
		INSTANCE_->show_cursor_impl(value);
	}

protected:
	virtual bool is_key_pressed_impl(keymap::WKEY keycode) const = 0;
	virtual bool is_mouse_button_pressed_impl(keymap::WMOUSE button) const = 0;
	virtual std::pair<float,float> get_mouse_position_impl() const = 0;
	virtual void set_mouse_position_impl(float x, float y) const = 0;
	virtual void center_mouse_position_impl() const = 0;
	virtual void show_cursor_impl(bool value) const = 0;

private:
	static void shutdown() { delete INSTANCE_; INSTANCE_ = nullptr; }
	static bool parse_keybindings(void* node);

private:
	static Input* INSTANCE_;
};


} // namespace erwin