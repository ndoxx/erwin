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
	SceneHierarchyWidget(erwin::Scene& scene);
	virtual ~SceneHierarchyWidget();

protected:
	virtual void on_imgui_render() override;

private:
	erwin::Scene& scene_;
};

} // namespace editor