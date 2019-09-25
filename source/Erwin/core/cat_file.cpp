#include <fstream>
#include <iostream>

#include "core/core.h"
#include "core/cat_file.h"

namespace erwin
{

void read_cat(CATDescriptor& desc)
{
    std::ifstream ifs(desc.filepath, std::ios::binary);

    // Read header & sanity check
    CATHeaderWrapper header;
    ifs.read(reinterpret_cast<char*>(&header), CAT_HEADER_SIZE);

    W_ASSERT(header.h.magic == CAT_MAGIC, "Invalid CAT file: magic number mismatch.");
    W_ASSERT(header.h.version_major == CAT_VERSION_MAJOR, "Invalid CAT file: version (major) mismatch.");
    W_ASSERT(header.h.version_minor == CAT_VERSION_MINOR, "Invalid CAT file: version (minor) mismatch.");

    desc.texture_width        = header.h.texture_width;
    desc.texture_height       = header.h.texture_height;
    desc.texture_blob_size    = header.h.texture_blob_size;
    desc.remapping_blob_size  = header.h.remapping_blob_size;
    desc.texture_compression  = (TextureCompression)header.h.texture_compression;
    desc.lossless_compression = (LosslessCompression)header.h.lossless_compression;

    // Read data blobs
    desc.texture_blob = new char[desc.texture_blob_size];
    ifs.read(reinterpret_cast<char*>(desc.texture_blob), desc.texture_blob_size);

    desc.remapping_blob = new char[desc.remapping_blob_size];
    ifs.read(reinterpret_cast<char*>(desc.remapping_blob), desc.remapping_blob_size);
}

void CATDescriptor::release()
{
	delete[] static_cast<char*>(texture_blob);
	delete[] static_cast<char*>(remapping_blob);
}

void write_cat(const CATDescriptor& desc)
{
    CATHeaderWrapper header;
    header.h.magic                = CAT_MAGIC;
    header.h.version_major        = CAT_VERSION_MAJOR;
    header.h.version_minor        = CAT_VERSION_MINOR;
    header.h.texture_width        = (uint16_t)desc.texture_width;
    header.h.texture_height       = (uint16_t)desc.texture_height;
    header.h.texture_blob_size    = desc.texture_blob_size;
    header.h.remapping_blob_size  = desc.remapping_blob_size;
    header.h.texture_compression  = (uint16_t)desc.texture_compression;
    header.h.lossless_compression = (uint16_t)desc.lossless_compression;

    std::ofstream ofs(desc.filepath, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(&header), CAT_HEADER_SIZE);
    ofs.write(reinterpret_cast<const char*>(desc.texture_blob), desc.texture_blob_size);
    ofs.write(reinterpret_cast<const char*>(desc.remapping_blob), desc.remapping_blob_size);
    ofs.close();
}

void traverse_remapping(const CATDescriptor& desc, std::function<void(const CATAtlasRemapElement&)> visit)
{
	uint32_t num_remap = desc.remapping_blob_size / sizeof(CATAtlasRemapElement);
	for(int ii=0; ii<num_remap; ++ii)
		visit(*(reinterpret_cast<const CATAtlasRemapElement*>(desc.remapping_blob) + ii));
}


} // namespace erwin