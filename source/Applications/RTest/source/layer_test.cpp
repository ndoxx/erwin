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

    Texture2DDescriptor desc;
    TextureHandle tex = AssetManager::load_image("textures/map/upack/beachSand/albedo.png", desc);
    futures_.emplace_back(Renderer::get_pixel_data(tex));
}

void LayerTest::on_detach()
{

}

void LayerTest::on_update(GameClock& clock)
{
	for(auto it=futures_.begin(); it!=futures_.end(); )
	{
		auto&& fut = *it;
		if(is_ready(fut))
		{
			auto&& [data, size] = fut.get();

			for(size_t ii=0; ii<size/100; ++ii)
			{
				DLOG("editor",1) << int(data[ii]) << " ";
			}
			DLOG("editor",1) << std::endl;

			futures_.erase(it);
		}
		else
			++it;
	}
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
