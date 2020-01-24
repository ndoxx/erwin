#pragma once

#include "widget.h"

namespace game
{
	class Scene;
}

namespace editor
{

class CameraTrackerOverlay: public Widget
{
public:
	CameraTrackerOverlay(game::Scene& scene);
	virtual ~CameraTrackerOverlay();

protected:
	virtual void on_imgui_render() override;

private:
	game::Scene& scene_;
};

} // namespace editor