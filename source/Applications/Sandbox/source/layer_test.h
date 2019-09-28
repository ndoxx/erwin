#pragma once

#include "erwin.h"
#include "render/render_device.h"

using namespace erwin;


class LayerTest: public Layer
{
public:
	LayerTest():
	Layer("LayerTest")
	{

	}

	~LayerTest() = default;

	virtual void on_imgui_render() override
	{

	}

	virtual void on_attach() override
	{
	/*
		FrameBufferLayout layout_0 =
		{
			{"target1"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::REPEAT}
		};
		Gfx::framebuffer_pool->create_framebuffer("fb_ratio_05"_h, make_scope<FbRatioConstraint>(0.5f,0.5f), layout_0, false);

		FrameBufferLayout layout_1 =
		{
			{"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::REPEAT},
			{"normal"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::REPEAT}
		};
		Gfx::framebuffer_pool->create_framebuffer("fb_ratio_025_05"_h, make_scope<FbRatioConstraint>(0.25f,0.5f), layout_1, true);
	*/
	}


protected:
	virtual void on_update(GameClock& clock) override
	{

	}


private:

};