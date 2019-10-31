#include "render/framebuffer_layout.h"

#include <cstring>

namespace erwin
{

FramebufferLayout::FramebufferLayout(FramebufferLayoutElement* elements, uint32_t count)
{
	elements_.resize(count);
	memcpy(elements_.data(), elements, count * sizeof(FramebufferLayoutElement));
}

} // namspace erwin