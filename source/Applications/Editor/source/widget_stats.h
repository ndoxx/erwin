#pragma once

#include "widget.h"

namespace editor
{

class RenderStatsWidget: public Widget
{
public:
	RenderStatsWidget();
	virtual ~RenderStatsWidget();

	virtual void on_update() override;

protected:
	virtual void on_imgui_render() override;

private:

};

} // namespace editor