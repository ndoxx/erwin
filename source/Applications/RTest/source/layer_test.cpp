#include "layer_test.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

using namespace erwin;

static struct
{
	CubemapHandle cubemap;
} s_storage;

LayerTest::LayerTest(): Layer("TestLayer")
{

}

void LayerTest::on_imgui_render()
{
	
}

void LayerTest::on_attach()
{
	CubemapDescriptor desc
	{
	    512,
	    512,
	    {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr},
	    ImageFormat::RGB16F,
	    MIN_LINEAR | MAG_LINEAR,
	    TextureWrap::CLAMP_TO_EDGE,
	    false
	};

	s_storage.cubemap = Renderer::create_cubemap(desc);
}

void LayerTest::on_detach()
{
	Renderer::destroy(s_storage.cubemap);
}

void LayerTest::on_update(GameClock& clock)
{

}

void LayerTest::on_render()
{

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
