#include "entity/component/bounding_box.h"

namespace erwin
{

ComponentOBB::ComponentOBB():
extent_m(-1.f,1.f,-1.f,1.f,-1.f,1.f),
display(false)
{
	std::tie(offset, half) = bound::to_vectors(extent_m);
}

ComponentOBB::ComponentOBB(const Extent& extent):
extent_m(extent),
display(false)
{
	std::tie(offset, half) = bound::to_vectors(extent_m);
}

} // namespace erwin