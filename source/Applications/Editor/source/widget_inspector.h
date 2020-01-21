#pragma once

#include "widget.h"

namespace editor
{

class Scene;
class InspectorWidget: public Widget
{
public:
	InspectorWidget(Scene& scene);
	virtual ~InspectorWidget();

protected:
	virtual void on_imgui_render() override;
	void environment_tab();
	void postproc_tab();

private:
	Scene& scene_;
};

} // namespace editor