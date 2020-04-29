#pragma once

#include "widget/widget.h"
#include "render/handles.h"

namespace editor
{

class MaterialAuthoringWidget: public Widget
{
public:
	MaterialAuthoringWidget();
	virtual ~MaterialAuthoringWidget();

protected:
	virtual void on_imgui_render() override;

private:
	erwin::TextureHandle checkerboard_tex_;
};

} // namespace editor