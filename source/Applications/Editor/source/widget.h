#pragma once

#include <string>

namespace editor
{

class Widget
{
public:
	Widget(const std::string& name, bool open);
	virtual ~Widget() = default;

	void render();

	inline void show(bool value=true) { open_ = value; }
	inline void hide(bool value=true) { open_ = !value; }
	inline const std::string& get_name() const { return name_; }

	bool open_;

protected:
	virtual void on_render() = 0;
	virtual void on_resize(uint32_t width, uint32_t height) { }

protected:
	std::string name_;
	uint32_t width_;
	uint32_t height_;
};


} // namespace editor