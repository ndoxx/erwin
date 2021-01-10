#pragma once

#include "graphics.h"

using namespace gfx;

class GfxTestApplication
{
public:
	GfxTestApplication(const std::string& window_title, uint32_t width, uint32_t height);
	virtual ~GfxTestApplication() = default;
	bool init(DeviceAPI api);
	void run();
	void shutdown();

protected:
	virtual void update() = 0;

protected:
    std::unique_ptr<Window> window_;
    std::unique_ptr<RenderDevice> render_device_;
    std::unique_ptr<SwapChain> swap_chain_;
    bool is_running_ = true;
    uint32_t window_width_;
    uint32_t window_height_;
    std::string window_title_;
};