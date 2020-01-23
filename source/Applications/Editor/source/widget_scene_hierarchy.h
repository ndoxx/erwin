#pragma once

#include "widget.h"

namespace game
{
	class Scene;
}

namespace editor
{

class SceneHierarchyWidget: public Widget
{
public:
	SceneHierarchyWidget(game::Scene& scene);
	virtual ~SceneHierarchyWidget();

protected:
	virtual void on_imgui_render() override;

private:
	game::Scene& scene_;
};

} // namespace editor