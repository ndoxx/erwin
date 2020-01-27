#pragma once

#include "editor/widget.h"

namespace erwin
{
	class Scene;
}

namespace editor
{

class SceneHierarchyWidget: public Widget
{
public:
	SceneHierarchyWidget();
	virtual ~SceneHierarchyWidget();

protected:
	virtual void on_imgui_render() override;
};

} // namespace editor