#include "layer_test.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

using namespace erwin;

static struct
{
	FramebufferHandle fb;
} s_storage;

LayerTest::LayerTest(): Layer("TestLayer")
{

}

void LayerTest::on_imgui_render()
{
	
}

void LayerTest::on_attach()
{
	FramebufferLayout layout
	{
	    {"cubemap"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE}
	};
	s_storage.fb = FramebufferPool::create_framebuffer("cmfb"_h, make_scope<FbFixedConstraint>(512, 512), FB_CUBEMAP_ATTACHMENT, layout);
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
