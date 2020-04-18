#include "filesystem/hdr_file.h"
#include "stb/stb_image.h"
#include "debug/logger.h"

#include <cstring>

namespace erwin
{
namespace hdr
{

void read_hdr(HDRDescriptor& desc)
{
	stbi_set_flip_vertically_on_load(true);

	int x, y, n;
	float* data = stbi_loadf(desc.filepath.string().c_str(), &x, &y, &n, int(desc.channels));

	// TMP: I need to perform deallocation myself in the renderer
	// so data is copied
	desc.data = new float[size_t(x*y*n)];
	memcpy(desc.data, data, size_t(x*y*n)*sizeof(float));
	desc.width = uint32_t(x);
	desc.height = uint32_t(y);
	if(desc.channels == 0)
		desc.channels = uint32_t(n);

	stbi_image_free(data);
}

} // namespace hdr
} // namespace erwin