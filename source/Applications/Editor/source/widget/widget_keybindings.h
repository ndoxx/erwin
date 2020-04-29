#pragma once

#include "widget/widget.h"

namespace editor
{

class KeybindingsWidget: public Widget
{
public:
	KeybindingsWidget();
	virtual ~KeybindingsWidget() = default;

	virtual bool on_keyboard_event(const erwin::KeyboardEvent& event);

protected:
	virtual void on_imgui_render() override;

private:
	uint32_t selection_;
};

} // namespace editor