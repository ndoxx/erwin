#pragma once

#include "widget/widget.h"

namespace erwin
{
	class Scene;
}

namespace editor
{

class CameraTrackerOverlay: public Widget
{
public:
	CameraTrackerOverlay();
	virtual ~CameraTrackerOverlay() = default;

protected:
	virtual void on_imgui_render() override;
};

} // namespace editor