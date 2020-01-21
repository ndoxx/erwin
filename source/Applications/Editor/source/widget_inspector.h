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
	virtual void on_render() override;

private:
	Scene& scene_;
};

} // namespace editor