#pragma once

#include <string>
#include "event/window_events.h"
#include "core/game_clock.h"

namespace editor
{

class Widget
{
public:
	Widget(const std::string& name, bool open);
	virtual ~Widget() = default;

	virtual void on_update(const erwin::GameClock&) { }
	virtual void on_layer_render() { }

	void imgui_render();

	inline void show(bool value=true) { open_ = value; }
	inline void hide(bool value=true) { open_ = !value; }
	inline const std::string& get_name() const { return name_; }

	inline bool is_hovered() const { return is_hovered_; }

	inline void save_state_and_hide()
	{
		was_open_ = open_;
		open_ = false;
	}
	inline void restore()
	{
		open_ = was_open_;
	}

	bool open_;

protected:
	virtual void on_imgui_render() = 0;
	virtual void on_resize(uint32_t /*width*/, uint32_t /*height*/) { }
	virtual void on_move(int32_t /*x_pos*/, int32_t /*y_pos*/) { }

protected:
	std::string name_;
	std::string dbg_profile_name_;
	int flags_;
	uint32_t width_;
	uint32_t height_;
	int32_t x_pos_;
	int32_t y_pos_;
	bool has_focus_;
	bool is_hovered_;
	bool was_open_;
};


} // namespace editor