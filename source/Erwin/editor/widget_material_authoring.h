#pragma once

#include "editor/widget.h"

namespace editor
{

class MaterialAuthoringWidget: public Widget
{
public:
	MaterialAuthoringWidget();
	virtual ~MaterialAuthoringWidget();

	virtual void on_update() override;
	

protected:
	virtual void on_imgui_render() override;
	virtual void on_resize(uint32_t width, uint32_t height) override;
	virtual void on_move(int32_t x_pos, int32_t y_pos) override;

private:
	struct RenderSurface
	{
		float x0;
		float y0;
		float x1;
		float y1;
		float w;
		float h;
	} render_surface_;
};

} // namespace editor