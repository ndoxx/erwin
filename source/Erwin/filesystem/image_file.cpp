#include "filesystem/image_file.h"
#include "stb/stb_image.h"
#include "debug/logger.h"

#include <cstring>

namespace erwin
{
namespace img
{

void read_hdr(HDRDescriptor& desc)
{
	stbi_set_flip_vertically_on_load(true);

	int x, y, n;
	float* data = stbi_loadf(desc.filepath.string().c_str(), &x, &y, &n, int(desc.channels));

	// TMP: I need to perform deallocation myself in the renderer
	// so data is copied
	if(desc.channels == 0)
		desc.channels = uint32_t(n);

	int channels = int(desc.channels);
	desc.data = new float[size_t(x*y*channels)];
	memcpy(desc.data, data, size_t(x*y*channels)*sizeof(float));
	desc.width = uint32_t(x);
	desc.height = uint32_t(y);

	stbi_image_free(data);
}

void read_png(PNGDescriptor& desc)
{
	stbi_set_flip_vertically_on_load(true);

	int x, y, n;
	unsigned char* data = stbi_load(desc.filepath.string().c_str(), &x, &y, &n, int(desc.channels));

	// TMP: I need to perform deallocation myself in the renderer
	// so data is copied
	if(desc.channels == 0)
		desc.channels = uint32_t(n);

	int channels = int(desc.channels);
	desc.data = new unsigned char[size_t(x*y*channels)];
	memcpy(desc.data, data, size_t(x*y*channels)*sizeof(unsigned char));
	desc.width = uint32_t(x);
	desc.height = uint32_t(y);

	stbi_image_free(data);
}

} // namespace img
} // namespace erwin