#pragma once

#include "widget.h"

namespace game
{
	class Scene;
}

namespace erwin
{
	class EntityManager;
}

namespace editor
{

class InspectorWidget: public Widget
{
public:
	InspectorWidget(game::Scene& scene, erwin::EntityManager& emgr);
	virtual ~InspectorWidget();

protected:
	virtual void on_imgui_render() override;
	void postproc_tab();
	void entity_tab();

private:
	game::Scene& scene_;
	erwin::EntityManager& entity_manager_;
};

} // namespace editor