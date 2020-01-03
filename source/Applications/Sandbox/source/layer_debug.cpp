#include "layer_debug.h"

#include <iostream>
#include <iomanip>
#include <bitset>

using namespace erwin;

DebugLayer::DebugLayer(): Layer("DebugLayer")
{

}

void DebugLayer::on_imgui_render()
{

}

void DebugLayer::on_attach()
{

}

void DebugLayer::on_detach()
{

}

void DebugLayer::on_update(GameClock& clock)
{

}

bool DebugLayer::on_event(const MouseButtonEvent& event)
{
	return false;
}

bool DebugLayer::on_event(const WindowResizeEvent& event)
{
	return false;
}

bool DebugLayer::on_event(const MouseScrollEvent& event)
{
	return false;
}
