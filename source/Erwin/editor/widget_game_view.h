#pragma once

#include "editor/widget.h"

namespace erwin
{
	class Scene;
	class EntityManager;
}

namespace editor
{

class GameViewWidget: public Widget
{
public:
	GameViewWidget();
	virtual ~GameViewWidget();

	virtual void on_update() override;
	
	bool on_mouse_event(const erwin::MouseButtonEvent& event);

protected:
	virtual void on_imgui_render() override;
	virtual void on_resize(uint32_t width, uint32_t height) override;
	virtual void on_move(int32_t x_pos, int32_t y_pos) override;

	void frame_profiler_window(bool* p_open);

private:
	Widget* stats_overlay_;
	Widget* camera_overlay_;

	struct RenderSurface
	{
		float x0;
		float y0;
		float x1;
		float y1;
		float w;
		float h;
	} render_surface_;

	bool enable_runtime_profiling_;
	bool track_next_frame_draw_calls_;
};

} // namespace editor