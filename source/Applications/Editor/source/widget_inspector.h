#pragma once

#include "widget.h"

namespace game
{
	class Scene;
}

namespace editor
{

class InspectorWidget: public Widget
{
public:
	InspectorWidget(game::Scene& scene);
	virtual ~InspectorWidget();

protected:
	virtual void on_imgui_render() override;
	void environment_tab();
	void postproc_tab();

private:
	game::Scene& scene_;
};

} // namespace editor