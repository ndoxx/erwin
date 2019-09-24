#include <fstream>
#include <iostream>

#include "core/core.h"
#include "core/dxa_file.h"

namespace erwin
{

void read_dxa(DXADescriptor& desc)
{
    std::ifstream ifs(desc.filepath, std::ios::binary);

    // Read header & sanity check
    DXAHeaderWrapper header;
    ifs.read(reinterpret_cast<char*>(&header), DXA_HEADER_SIZE);

    W_ASSERT(header.h.magic == DXA_MAGIC, "Invalid DXA file: magic number mismatch.");
    W_ASSERT(header.h.version_major == DXA_VERSION_MAJOR, "Invalid DXA file: version (major) mismatch.");
    W_ASSERT(header.h.version_minor == DXA_VERSION_MINOR, "Invalid DXA file: version (minor) mismatch.");

    desc.texture_width       = header.h.texture_width;
    desc.texture_height      = header.h.texture_height;
    desc.texture_blob_size   = header.h.texture_blob_size;
    desc.remapping_blob_size = header.h.remapping_blob_size;

    // Read data blobs
    desc.texture_blob = new char[desc.texture_blob_size];
    ifs.read(reinterpret_cast<char*>(desc.texture_blob), desc.texture_blob_size);

    desc.remapping_blob = new char[desc.remapping_blob_size];
    ifs.read(reinterpret_cast<char*>(desc.remapping_blob), desc.remapping_blob_size);
}

void DXADescriptor::release()
{
	delete[] static_cast<char*>(texture_blob);
	delete[] static_cast<char*>(remapping_blob);
}

void write_dxa(const DXADescriptor& desc)
{
    DXAHeaderWrapper header;
    header.h.magic               = DXA_MAGIC;
    header.h.version_major       = DXA_VERSION_MAJOR;
    header.h.version_minor       = DXA_VERSION_MINOR;
    header.h.texture_width       = desc.texture_width;
    header.h.texture_height      = desc.texture_height;
    header.h.texture_blob_size   = desc.texture_blob_size;
    header.h.remapping_blob_size = desc.remapping_blob_size;

    std::ofstream ofs(desc.filepath, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(&header), DXA_HEADER_SIZE);
    ofs.write(reinterpret_cast<const char*>(desc.texture_blob), desc.texture_blob_size);
    ofs.write(reinterpret_cast<const char*>(desc.remapping_blob), desc.remapping_blob_size);
    ofs.close();
}

void traverse_remapping(const DXADescriptor& desc, std::function<void(const DXAAtlasRemapElement&)> visit)
{
	uint32_t num_remap = desc.remapping_blob_size / sizeof(DXAAtlasRemapElement);
	for(int ii=0; ii<num_remap; ++ii)
		visit(*(reinterpret_cast<const DXAAtlasRemapElement*>(desc.remapping_blob) + ii));
}


} // namespace erwin