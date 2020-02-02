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
	InspectorWidget();
	virtual ~InspectorWidget();

protected:
	virtual void on_imgui_render() override;
	void entity_tab();
};

} // namespace editor