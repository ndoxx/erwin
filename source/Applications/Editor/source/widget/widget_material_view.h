#pragma once

#include <memory>
#include "widget/widget.h"
#include "entity/component_PBR_material.h"
#include "entity/component_transform.h"
#include "entity/component_camera.h"
#include "entity/light.h"
#include "input/tracker_camera_system.h"

namespace editor
{

class MaterialViewWidget: public Widget
{
public:
	MaterialViewWidget();
	virtual ~MaterialViewWidget();

	virtual void on_update(const erwin::GameClock& clock) override;
	virtual void on_layer_render() override;

	inline bool on_event(const erwin::MouseButtonEvent& event)	{ return camera_controller_.on_mouse_button_event(event); }
	inline bool on_event(const erwin::WindowResizeEvent& event)	{ return camera_controller_.on_window_resize_event(event); }
	inline bool on_event(const erwin::WindowMovedEvent& event)	{ return camera_controller_.on_window_moved_event(event); }
	inline bool on_event(const erwin::MouseScrollEvent& event)	{ return camera_controller_.on_mouse_scroll_event(event); }
	inline bool on_event(const erwin::MouseMovedEvent& event)	{ return camera_controller_.on_mouse_moved_event(event); }
	inline bool on_event(const erwin::KeyboardEvent& event)		{ return camera_controller_.on_keyboard_event(event); }

	inline void set_material(std::shared_ptr<erwin::ComponentPBRMaterial> mat) { current_material_ = mat; }
	inline std::shared_ptr<erwin::ComponentPBRMaterial> get_material_shared()  { return current_material_; }
	void reset_material();

protected:
	virtual void on_imgui_render() override;
	virtual void on_resize(uint32_t width, uint32_t height) override;
	virtual void on_move(int32_t x_pos, int32_t y_pos) override;

private:
	struct RenderSurface
	{
		float x0;
		float y0;
		float x1;
		float y1;
		float w;
		float h;
	} render_surface_;

	size_t current_index_;

	std::shared_ptr<erwin::ComponentPBRMaterial> current_material_;
	erwin::ComponentTransform3D transform_;
	erwin::ComponentDirectionalLight directional_light_;
	erwin::ComponentCamera3D camera_;
	erwin::ComponentTransform3D camera_transform_;
	erwin::TrackerCameraSystem camera_controller_;
	erwin::VertexArrayHandle current_mesh_;
};

} // namespace editor