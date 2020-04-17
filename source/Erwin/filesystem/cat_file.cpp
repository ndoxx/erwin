#include <fstream>
#include <cstring>
#include <iostream>

#include "filesystem/cat_file.h"
#include "filesystem/filesystem.h"
#include "core/core.h"
#include "core/z_wrapper.h"

namespace erwin
{
namespace cat
{

// CAT file format
//#pragma pack(push,1)
struct CATHeader
{
    uint32_t magic;                 // Magic number to check file format validity
    uint16_t version_major;         // Version major number
    uint16_t version_minor;         // Version minor number
    uint16_t texture_width;         // Width of texture in pixels
    uint16_t texture_height;        // Height of texture in pixels
    uint16_t texture_compression;   // Type of (lossy) texture compression
    uint16_t lossless_compression;  // Type of (lossless) blob compression
    uint16_t remapping_type;        // Texture atlas or font remapping?
    uint64_t texture_blob_size;     // Size of texture blob
    uint64_t blob_inflate_size;     // Size of inflated texture blob (after blob decompression)
    uint64_t remapping_blob_size;   // Size of remapping table
};
//#pragma pack(pop)

#define CAT_MAGIC 0x54414357 // ASCII(WCAT)
#define CAT_VERSION_MAJOR 1
#define CAT_VERSION_MINOR 2

// Helper functions to (de)allocate data blobs inside the filesystem resource arena if possible, on the heap if not
static inline uint8_t* new_blob(std::size_t size)
{
    if(filesystem::is_arena_initialized())
    {
        return W_NEW_ARRAY_DYNAMIC(uint8_t, size, filesystem::get_arena());
    }
    else
        return new uint8_t[size];
}
static inline void delete_blob(void* ptr)
{
    if(filesystem::is_arena_initialized())
    {
        W_DELETE_ARRAY(static_cast<uint8_t*>(ptr), filesystem::get_arena());
    }
    else
        delete[] static_cast<uint8_t*>(ptr);
}

void read_cat(CATDescriptor& desc)
{
    std::ifstream ifs(desc.filepath, std::ios::binary);

    // Read header & sanity check
    CATHeader header;
    ifs.read(opaque_cast(&header), sizeof(CATHeader));

    W_ASSERT(header.magic == CAT_MAGIC, "Invalid CAT file: magic number mismatch.");
    W_ASSERT(header.version_major == CAT_VERSION_MAJOR, "Invalid CAT file: version (major) mismatch.");
    W_ASSERT(header.version_minor == CAT_VERSION_MINOR, "Invalid CAT file: version (minor) mismatch.");

    desc.texture_width        = header.texture_width;
    desc.texture_height       = header.texture_height;
    desc.texture_blob_size    = uint32_t(header.texture_blob_size);
    desc.remapping_blob_size  = uint32_t(header.remapping_blob_size);
    desc.texture_compression  = TextureCompression(header.texture_compression);
    desc.lossless_compression = LosslessCompression(header.lossless_compression);
    desc.remapping_type       = RemappingType(header.remapping_type);

    // Read data blobs
    uint8_t* texture_blob = new_blob(desc.texture_blob_size);
    ifs.read(opaque_cast(texture_blob), desc.texture_blob_size);
    // Inflate (decompress) blob if needed
    if(desc.lossless_compression == LosslessCompression::Deflate)
    {
        uint8_t* inflated = new_blob(header.blob_inflate_size);
        erwin::uncompress_data(texture_blob, int(desc.texture_blob_size), inflated, int(header.blob_inflate_size));
        desc.texture_blob = inflated;
        // delete[] texture_blob;
        delete_blob(texture_blob);
    }
    else
    {
        desc.texture_blob = texture_blob;
    }

    desc.remapping_blob = static_cast<void*>(new_blob(desc.remapping_blob_size));
    ifs.read(static_cast<char*>(desc.remapping_blob), desc.remapping_blob_size);

    ifs.close();
}

void CATDescriptor::release()
{
    delete_blob(texture_blob);
    delete_blob(remapping_blob);
}

void write_cat(const CATDescriptor& desc)
{
    CATHeader header;
    header.magic                = CAT_MAGIC;
    header.version_major        = CAT_VERSION_MAJOR;
    header.version_minor        = CAT_VERSION_MINOR;
    header.texture_width        = uint16_t(desc.texture_width);
    header.texture_height       = uint16_t(desc.texture_height);
    header.remapping_blob_size  = desc.remapping_blob_size;
    header.texture_compression  = uint16_t(desc.texture_compression);
    header.lossless_compression = uint16_t(desc.lossless_compression);
    header.remapping_type       = uint16_t(desc.remapping_type);

    std::ofstream ofs(desc.filepath, std::ios::binary);

    if(desc.lossless_compression == LosslessCompression::Deflate)
    {
        uint32_t max_size = uint32_t(erwin::get_max_compressed_len(int(desc.texture_blob_size)));
        uint8_t* deflated = new uint8_t[max_size];
        uint32_t comp_size = uint32_t(erwin::compress_data(static_cast<uint8_t*>(desc.texture_blob), int(desc.texture_blob_size), deflated, int(max_size)));
        
        header.texture_blob_size = comp_size;
        header.blob_inflate_size = desc.texture_blob_size;

        ofs.write(opaque_cast(&header), sizeof(CATHeader));
        ofs.write(opaque_cast(deflated), comp_size);
        delete[] deflated;
    }
    else
    {
        header.texture_blob_size = desc.texture_blob_size;
        header.blob_inflate_size = desc.texture_blob_size;

        ofs.write(opaque_cast(&header), sizeof(CATHeader));
        ofs.write(static_cast<const char*>(desc.texture_blob), desc.texture_blob_size);
    }

    ofs.write(static_cast<const char*>(desc.remapping_blob), desc.remapping_blob_size);
    ofs.close();
}

void traverse_texture_remapping(const CATDescriptor& desc, std::function<void(const CATAtlasRemapElement&)> visit)
{
	uint32_t num_remap = desc.remapping_blob_size / sizeof(CATAtlasRemapElement);
	for(uint32_t ii=0; ii<num_remap; ++ii)
		visit(*(reinterpret_cast<const CATAtlasRemapElement*>(desc.remapping_blob) + ii));
}

void traverse_font_remapping(const CATDescriptor& desc, std::function<void(const CATFontRemapElement&)> visit)
{
    uint32_t num_remap = desc.remapping_blob_size / sizeof(CATFontRemapElement);
    for(uint32_t ii=0; ii<num_remap; ++ii)
        visit(*(reinterpret_cast<const CATFontRemapElement*>(desc.remapping_blob) + ii));
}


} // namespace cat
} // namespace erwin