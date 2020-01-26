#include "asset/bounding.h"

namespace erwin
{

#ifdef W_DEBUG
std::ostream& operator <<(std::ostream& stream, const Extent& dims)
{
	stream << "(" 
		   << "x: " << dims[0] << "->" << dims[1] << ", "
		   << "y: " << dims[2] << "->" << dims[3] << ", "
		   << "z: " << dims[4] << "->" << dims[5] << ")";
	return stream;
}
#endif

} // namespace erwin