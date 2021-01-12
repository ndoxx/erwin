#pragma once

#include <memory>
#include "../../swapchain.h"

namespace gfx
{

class Window;
class OGLSwapchain : public Swapchain
{
public:
	OGLSwapchain(const Window& window);
	~OGLSwapchain() = default;

	void present() override;

private:
	const Window& window_;
};

} // namespace gfx