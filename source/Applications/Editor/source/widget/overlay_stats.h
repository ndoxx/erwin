#pragma once

#include "widget/widget.h"

namespace editor
{

class RenderStatsOverlay: public Widget
{
public:
	RenderStatsOverlay();
	virtual ~RenderStatsOverlay() = default;

	virtual void on_update(const erwin::GameClock& clock) override;

protected:
	virtual void on_imgui_render() override;
};

} // namespace editor