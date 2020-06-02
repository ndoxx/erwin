#include "layer_test.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"
#include "render/renderer.h"
#include "asset/asset_manager.h"
#include "utils/future.hpp"

using namespace erwin;

LayerTest::LayerTest(): Layer("TestLayer")
{

}

void LayerTest::on_imgui_render()
{
}

void LayerTest::on_attach()
{
    wfs::set_asset_dir("source/Applications/Editor/assets");
}

void LayerTest::on_commit()
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

}

bool LayerTest::on_mouse_button_event(const MouseButtonEvent& event)
{
	return false;
}

bool LayerTest::on_window_resize_event(const WindowResizeEvent& event)
{
	return false;
}

bool LayerTest::on_mouse_scroll_event(const MouseScrollEvent& event)
{
	return false;
}
