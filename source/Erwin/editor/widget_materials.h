#pragma once

#include "editor/widget.h"

namespace erwin
{
	class Scene;
}

namespace editor
{

class MaterialsWidget: public Widget
{
public:
	MaterialsWidget();
	virtual ~MaterialsWidget() = default;

protected:
	virtual void on_imgui_render() override;
};

} // namespace editor