#pragma once

#include "editor/widget.h"

namespace editor
{

class RenderStatsOverlay: public Widget
{
public:
	RenderStatsOverlay();
	virtual ~RenderStatsOverlay() = default;

	virtual void on_update() override;

protected:
	virtual void on_imgui_render() override;
};

} // namespace editor