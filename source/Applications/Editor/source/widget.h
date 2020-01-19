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

	bool open_;

protected:
	virtual void on_render() = 0;

private:
	std::string name_;
};


} // namespace editor