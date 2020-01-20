#include "widget_game_view.h"
#include "erwin.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

GameViewWidget::GameViewWidget():
Widget("Game", true)
{

}

GameViewWidget::~GameViewWidget()
{

}

void GameViewWidget::on_resize(uint32_t width, uint32_t height)
{
	EVENTBUS.publish(WindowResizeEvent(width, height));
	EVENTBUS.publish(FramebufferResizeEvent(width, height));
}

void GameViewWidget::on_move(int32_t x, int32_t y)
{
	EVENTBUS.publish(WindowMovedEvent(x, y));
}

void GameViewWidget::on_render()
{
    // * Show game render in window
    float winx = std::max(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - 8.f, 0.f);
    float winy = std::max(ImGui::GetWindowPos().y + ImGui::GetWindowSize().y - 8.f, 0.f);

	// Retrieve the native framebuffer texture handle
	FramebufferHandle fb = FramebufferPool::get_framebuffer("game_view"_h);
	TextureHandle texture = Renderer::get_framebuffer_texture(fb, 0);
	void* framebuffer_texture_native = Renderer::get_native_texture_handle(texture);
    ImGui::GetWindowDrawList()->AddImage(framebuffer_texture_native,
                                         ImGui::GetCursorScreenPos(),
                                         ImVec2(winx, winy),
                                         ImVec2(0, 1), ImVec2(1, 0));
}


} // namespace editor