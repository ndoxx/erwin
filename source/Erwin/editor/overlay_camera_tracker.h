#pragma once

#include "editor/widget.h"

namespace erwin
{
	class Scene;
}

namespace editor
{

class CameraTrackerOverlay: public Widget
{
public:
	CameraTrackerOverlay(erwin::Scene& scene);
	virtual ~CameraTrackerOverlay();

protected:
	virtual void on_imgui_render() override;

private:
	erwin::Scene& scene_;
};

} // namespace editor