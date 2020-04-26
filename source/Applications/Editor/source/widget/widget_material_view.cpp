#include "widget/widget_material_view.h"
#include "event/event_bus.h"
#include "event/window_events.h"
#include "render/handles.h"
#include "render/renderer.h"
#include "render/framebuffer_pool.h"

#include "imgui.h"

using namespace erwin;

namespace editor
{

static constexpr float k_border = 4.f;
static constexpr float k_start_x = 4.f;
static constexpr float k_start_y = 43.f;

MaterialViewWidget::MaterialViewWidget():
Widget("Material view", true),
render_surface_{0.f,0.f,0.f,0.f,0.f,0.f}
{
	// flags_ |= ImGuiWindowFlags_MenuBar;
}

MaterialViewWidget::~MaterialViewWidget()
{

}

void MaterialViewWidget::on_update()
{

}

void MaterialViewWidget::on_resize(uint32_t width, uint32_t height)
{
	float rw = std::max(float(width)  - (k_border + k_start_x), 1.f);
	float rh = std::max(float(height) - (k_border + k_start_y), 1.f);
	render_surface_.x1 = render_surface_.x0 + rw;
	render_surface_.y1 = render_surface_.y0 + rh;
    render_surface_.w = rw;
    render_surface_.h = rh;

	EVENTBUS.publish(WindowResizeEvent(int(width), int(height)));
	EVENTBUS.publish(FramebufferResizeEvent(int(rw), int(rh)));
}

void MaterialViewWidget::on_move(int32_t x, int32_t y)
{
	render_surface_.x0 = float(x) + k_start_x;
	render_surface_.y0 = float(y) + k_start_y;

	EVENTBUS.publish(WindowMovedEvent(x, y));
}

void MaterialViewWidget::on_imgui_render()
{
    // * Show game render in window
	// Retrieve the native framebuffer texture handle
	FramebufferHandle fb = FramebufferPool::get_framebuffer("host"_h);
	TextureHandle texture = Renderer::get_framebuffer_texture(fb, 0);
	void* framebuffer_texture_native = Renderer::get_native_texture_handle(texture);
    ImGui::GetWindowDrawList()->AddImage(framebuffer_texture_native,
                                         // ImGui::GetCursorScreenPos(),
                                         ImVec2(render_surface_.x0, render_surface_.y0),
                                         ImVec2(render_surface_.x1, render_surface_.y1),
                                         ImVec2(0, 1), ImVec2(1, 0));
}

bool MaterialViewWidget::on_mouse_event(const erwin::MouseButtonEvent& event)
{
	return true;
}


} // namespace editor