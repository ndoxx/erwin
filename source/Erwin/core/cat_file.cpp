#include <fstream>
#include <cstring>
#include <iostream>

#include "core/core.h"
#include "core/cat_file.h"
#include "core/z_wrapper.h"

namespace erwin
{

void read_cat(CATDescriptor& desc)
{
    std::ifstream ifs(desc.filepath, std::ios::binary);

    // Read header & sanity check
    CATHeader header;
    ifs.read(reinterpret_cast<char*>(&header), sizeof(CATHeader));

    W_ASSERT(header.magic == CAT_MAGIC, "Invalid CAT file: magic number mismatch.");
    W_ASSERT(header.version_major == CAT_VERSION_MAJOR, "Invalid CAT file: version (major) mismatch.");
    W_ASSERT(header.version_minor == CAT_VERSION_MINOR, "Invalid CAT file: version (minor) mismatch.");

    desc.texture_width        = header.texture_width;
    desc.texture_height       = header.texture_height;
    desc.texture_blob_size    = header.texture_blob_size;
    desc.remapping_blob_size  = header.remapping_blob_size;
    desc.texture_compression  = (TextureCompression)header.texture_compression;
    desc.lossless_compression = (LosslessCompression)header.lossless_compression;

    // Read data blobs
    char* texture_blob = new char[desc.texture_blob_size];
    ifs.read(texture_blob, desc.texture_blob_size);
    // Inflate (decompress) blob if needed
    if(desc.lossless_compression == LosslessCompression::Deflate)
    {
        uint8_t* inflated = new uint8_t[header.blob_inflate_size];
        erwin::uncompress_data(reinterpret_cast<uint8_t*>(texture_blob), desc.texture_blob_size, inflated, header.blob_inflate_size);
        desc.texture_blob = inflated;
        delete[] texture_blob;
    }
    else
    {
        desc.texture_blob = texture_blob;
    }

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
    CATHeader header;
    header.magic                = CAT_MAGIC;
    header.version_major        = CAT_VERSION_MAJOR;
    header.version_minor        = CAT_VERSION_MINOR;
    header.texture_width        = (uint16_t)desc.texture_width;
    header.texture_height       = (uint16_t)desc.texture_height;
    header.remapping_blob_size  = desc.remapping_blob_size;
    header.texture_compression  = (uint16_t)desc.texture_compression;
    header.lossless_compression = (uint16_t)desc.lossless_compression;

    std::ofstream ofs(desc.filepath, std::ios::binary);

    if(desc.lossless_compression == LosslessCompression::Deflate)
    {
        uint32_t max_size = erwin::get_max_compressed_len(desc.texture_blob_size);
        uint8_t* deflated = new uint8_t[max_size];
        uint32_t comp_size = erwin::compress_data(reinterpret_cast<uint8_t*>(desc.texture_blob), desc.texture_blob_size, deflated, max_size);
        
        header.texture_blob_size = comp_size;
        header.blob_inflate_size = desc.texture_blob_size;

        ofs.write(reinterpret_cast<const char*>(&header), sizeof(CATHeader));
        ofs.write(reinterpret_cast<const char*>(deflated), comp_size);
        delete[] deflated;
    }
    else
    {
        header.texture_blob_size = desc.texture_blob_size;
        header.blob_inflate_size = desc.texture_blob_size;

        ofs.write(reinterpret_cast<const char*>(&header), sizeof(CATHeader));
        ofs.write(reinterpret_cast<const char*>(desc.texture_blob), desc.texture_blob_size);
    }

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