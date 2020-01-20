#include "layer_test.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

using namespace erwin;

LayerTest::LayerTest(): Layer("TestLayer")
{

}

void LayerTest::on_imgui_render()
{
	
}

void LayerTest::on_attach()
{

}

void LayerTest::on_detach()
{

}

void LayerTest::on_update(GameClock& clock)
{

}

void LayerTest::on_render()
{
	FramebufferHandle fb = FramebufferPool::get_framebuffer("game_view"_h);
	Renderer::clear(1, fb, ClearFlags::CLEAR_COLOR_FLAG, {1.0f,0.f,0.f,1.f});
}

bool LayerTest::on_event(const MouseButtonEvent& event)
{
	return false;
}

bool LayerTest::on_event(const WindowResizeEvent& event)
{
	return false;
}

bool LayerTest::on_event(const MouseScrollEvent& event)
{
	return false;
}
