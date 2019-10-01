#include "core/tom_file.h"
#include "core/core.h"
#include "core/z_wrapper.h"

#include <cstring>

namespace erwin
{
namespace tom
{

// TOM file format
//#pragma pack(push,1)
struct TOMHeader
{
    uint32_t magic;                 // Magic number to check file format validity
    uint16_t version_major;         // Version major number
    uint16_t version_minor;         // Version minor number
    uint16_t texture_width;         // Width of textures in pixels
    uint16_t texture_height;        // Height of textures in pixels
    uint8_t  address_U;				// Texture wrap parameter for U coordinate
    uint8_t  address_V;				// Texture wrap parameter for V coordinate
    uint16_t num_maps;              // Number of texture maps
    uint16_t blob_compression;      // Type of (lossless) blob compression
    uint64_t blob_size;             // Size of concat texture blob
    uint64_t blob_inflate_size;     // Size of inflated concat texture blob (after blob decompression)
};
//#pragma pack(pop)

#define TOM_MAGIC 0x4d4f5457 // ASCII(WTOM)
#define TOM_VERSION_MAJOR 1
#define TOM_VERSION_MINOR 0

//#pragma pack(push,1)
struct BlockDescriptor
{
	uint8_t filter;		 // Minification & Magnification filter
	uint8_t channels;	 // Number of color channels
	uint8_t compression; // Texture compression
	uint32_t size;		 // Size of texture data
	uint64_t name;		 // Hashed name of texture map ("albedo"_h, "normal"_h, ...)
};
//#pragma pack(pop)

void TextureMapDescriptor::release()
{
	delete[] data;
}

void TOMDescriptor::release()
{
	for(auto&& tmap: texture_maps)
		tmap.release();
}


void read_tom(TOMDescriptor& desc)
{
    std::ifstream ifs(desc.filepath, std::ios::binary);

    // Read header & sanity check
    TOMHeader header;
    ifs.read(reinterpret_cast<char*>(&header), sizeof(TOMHeader));

    W_ASSERT(header.magic == TOM_MAGIC, "Invalid TOM file: magic number mismatch.");
    W_ASSERT(header.version_major == TOM_VERSION_MAJOR, "Invalid TOM file: version (major) mismatch.");
    W_ASSERT(header.version_minor == TOM_VERSION_MINOR, "Invalid TOM file: version (minor) mismatch.");

    desc.width       = header.texture_width;
    desc.height      = header.texture_height;
    desc.compression = (LosslessCompression)header.blob_compression;
    desc.address_U   = (TextureWrap)header.address_U;
    desc.address_V   = (TextureWrap)header.address_V;

    uint32_t num_maps          = header.num_maps;
    uint64_t blob_size         = header.blob_size;
    uint64_t blob_inflate_size = header.blob_inflate_size;

    // Read block descriptors
	std::vector<BlockDescriptor> blocks;
    blocks.reserve(num_maps);
    ifs.read(reinterpret_cast<char*>(blocks.data()), num_maps*sizeof(BlockDescriptor));

    for(auto&& block: blocks)
    {
    	TextureMapDescriptor bdesc =
    	{
			(TextureFilter)block.filter,
			block.channels,
			(TextureCompression)block.compression,
			block.size,
			nullptr,
			(hash_t)block.name
    	};
    	desc.texture_maps.push_back(bdesc);
    }

    // Read data blob
    uint8_t* blob = new uint8_t[blob_size];
    ifs.read(reinterpret_cast<char*>(blob), blob_size);
    ifs.close();

    // Inflate (decompress) blob if needed
    if(desc.compression == LosslessCompression::Deflate)
    {
        uint8_t* inflated = new uint8_t[blob_inflate_size];
        erwin::uncompress_data(reinterpret_cast<uint8_t*>(blob), blob_size, inflated, blob_inflate_size);

        // Free compressed blob buffer and reassign
        delete[] blob;
        blob = inflated;
    }

    // Extract texture maps data
	uint64_t offset = 0;
    for(auto&& tmap: desc.texture_maps)
    {
    	tmap.data = new uint8_t[tmap.size];
    	memcpy(tmap.data, blob+offset, tmap.size);
    	offset += tmap.size;
    }

    // Cleanup
    delete[] blob;
}

void write_tom(const TOMDescriptor& desc)
{
	uint16_t num_maps = desc.texture_maps.size();

	// Compute total uncompressed blob size
	uint64_t blob_size = 0;
	for(auto&& tmap: desc.texture_maps)
		blob_size += tmap.size;

	// Process per-texture map data
	std::vector<BlockDescriptor> blocks;
	uint8_t* blob = new uint8_t[blob_size];
	uint64_t offset = 0;
	for(auto&& tmap: desc.texture_maps)
	{
		// Concatenate texture maps into one big buffer
		memcpy(blob+offset, tmap.data, tmap.size);
		offset += tmap.size;

		// Generate block descriptor
		BlockDescriptor block =
		{
			(uint8_t)tmap.filter,
			tmap.channels,
			(uint8_t)tmap.compression,
			tmap.size,
			(uint64_t)tmap.name
		};

		blocks.push_back(block);
	}

	// Generate header
	TOMHeader header;
	header.magic             = TOM_MAGIC;
	header.version_major     = TOM_VERSION_MAJOR;
	header.version_minor     = TOM_VERSION_MINOR;
	header.texture_width     = desc.width;
	header.texture_height    = desc.height;
	header.address_U         = (uint8_t)desc.address_U;
	header.address_V         = (uint8_t)desc.address_V;
	header.num_maps          = num_maps;
	header.blob_compression  = (uint16_t)desc.compression;
	header.blob_size         = blob_size;
    header.blob_inflate_size = blob_size;

    std::ofstream ofs(desc.filepath, std::ios::binary);

    // Compress blob if required
    if(desc.compression == LosslessCompression::Deflate)
    {
    	// DEFLATE compression
        uint32_t max_size = erwin::get_max_compressed_len(blob_size);
        uint8_t* deflated = new uint8_t[max_size];
        header.blob_size  = erwin::compress_data(reinterpret_cast<uint8_t*>(blob), blob_size, deflated, max_size);
        header.blob_inflate_size = blob_size;

        // Free uncompressed blob buffer and reassign
        delete[] blob;
        blob = deflated;
    }

    // Write header
    ofs.write(reinterpret_cast<const char*>(&header), sizeof(TOMHeader));
    // Write block descriptors
    ofs.write(reinterpret_cast<const char*>(blocks.data()), num_maps*sizeof(BlockDescriptor));
    // Write blob
    ofs.write(reinterpret_cast<const char*>(blob), header.blob_size);

    ofs.close();

    // Cleanup
    delete[] blob;
}

std::vector<WRef<Texture2D>> make_textures(TOMDescriptor& desc)
{
	std::vector<WRef<Texture2D>> textures;

	return textures;
}

} // namespace tom
} // namespace erwin