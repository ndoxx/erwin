#pragma once

#include "widget/widget.h"

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

};

} // namespace editor