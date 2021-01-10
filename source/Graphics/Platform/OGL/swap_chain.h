#pragma once

#include <memory>
#include "../../swap_chain.h"

namespace gfx
{

class OGLSwapChain : public SwapChain
{
public:
	OGLSwapChain(const Window& window);
	~OGLSwapChain() = default;

	void present() override;

private:
	const Window& window_;
};

} // namespace gfx