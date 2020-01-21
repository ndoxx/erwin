#pragma once

#include "widget.h"

namespace editor
{

class Scene;
class SceneHierarchyWidget: public Widget
{
public:
	SceneHierarchyWidget(Scene& scene);
	virtual ~SceneHierarchyWidget();

protected:
	virtual void on_render() override;

private:
	Scene& scene_;
};

} // namespace editor