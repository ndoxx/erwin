#pragma once

#include "editor/widget.h"

namespace erwin
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
	InspectorWidget(erwin::Scene& scene, erwin::EntityManager& emgr);
	virtual ~InspectorWidget();

protected:
	virtual void on_imgui_render() override;
	void postproc_tab();
	void entity_tab();

private:
	erwin::Scene& scene_;
	erwin::EntityManager& entity_manager_;
};

} // namespace editor