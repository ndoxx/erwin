#include "filesystem/tom_file.h"
#include "filesystem/filesystem.h"
#include "asset/dxt_compressor.h"
#include "core/core.h"
#include "core/z_wrapper.h"
#include "debug/logger.h"

#include <cstring>
#include <numeric>

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
    uint16_t address_UV;			// Texture wrap parameter
    uint16_t num_maps;              // Number of texture maps
    uint16_t blob_compression;      // Type of (lossless) blob compression
    uint64_t blob_size;             // Size of concat texture blob
    uint64_t blob_inflate_size;     // Size of inflated concat texture blob (after blob decompression)
    uint64_t material_data_size;    // Size of material data structure
    uint8_t  material_type;         // Type of material
};
//#pragma pack(pop)

#define TOM_MAGIC 0x4d4f5457 // ASCII(WTOM)
#define TOM_VERSION_MAJOR 1
#define TOM_VERSION_MINOR 1

//#pragma pack(push,1)
struct BlockDescriptor
{
	uint8_t filter;		 // Minification & Magnification filter
	uint8_t channels;	 // Number of color channels
    uint8_t srgb;        // Use srgb?
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
    if(material_data)
        delete[] material_data;
}


void read_tom(TOMDescriptor& desc)
{
    auto ifs = wfs::get_istream(desc.filepath, wfs::binary);

    // Read header & sanity check
    TOMHeader header;
    ifs->read(opaque_cast(&header), sizeof(TOMHeader));

    W_ASSERT(header.magic == TOM_MAGIC, "Invalid TOM file: magic number mismatch.");
    W_ASSERT(header.version_major == TOM_VERSION_MAJOR, "Invalid TOM file: version (major) mismatch.");
    W_ASSERT(header.version_minor == TOM_VERSION_MINOR, "Invalid TOM file: version (minor) mismatch.");

    desc.width       = header.texture_width;
    desc.height      = header.texture_height;
    desc.compression = LosslessCompression(header.blob_compression);
    desc.address_UV  = TextureWrap(header.address_UV);

    // Read material data if any
    if(header.material_data_size)
    {
        desc.material_type = MaterialType(header.material_type);
        desc.material_data_size = uint32_t(header.material_data_size);
        desc.material_data = new uint8_t[header.material_data_size];
        ifs->read(opaque_cast(desc.material_data), long(header.material_data_size));
    }

    uint32_t num_maps          = header.num_maps;
    uint64_t blob_size         = header.blob_size;
    uint64_t blob_inflate_size = header.blob_inflate_size;

    // Read block descriptors
	std::vector<BlockDescriptor> blocks;
    blocks.resize(num_maps);
    ifs->read(opaque_cast(blocks.data()), num_maps*sizeof(BlockDescriptor));

    for(auto&& block: blocks)
    {
        // Sanity check
        W_ASSERT(block.channels>0, "Tom: wrong number of texture channels: min is 1.");
        W_ASSERT(block.channels<=4, "Tom: wrong number of texture channels: max is 4.");

    	TextureMapDescriptor bdesc =
    	{
			static_cast<TextureFilter>(block.filter),
			block.channels,
            bool(block.srgb),
			static_cast<TextureCompression>(block.compression),
			block.size,
			nullptr,
			static_cast<hash_t>(block.name)
    	};
    	desc.texture_maps.push_back(bdesc);
    }

    // Read data blob
    uint8_t* blob = new uint8_t[blob_size];
    ifs->read(opaque_cast(blob), long(blob_size));

    // Inflate (decompress) blob if needed
    if(desc.compression == LosslessCompression::Deflate)
    {
        uint8_t* inflated = new uint8_t[blob_inflate_size];
        erwin::uncompress_data(blob, int(blob_size), inflated, int(blob_inflate_size));

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

void write_tom(TOMDescriptor& desc)
{
	uint16_t num_maps = uint16_t(desc.texture_maps.size());

	// Compute total uncompressed blob size
    uint64_t blob_size =
        std::accumulate(desc.texture_maps.begin(), desc.texture_maps.end(), 0u,
                        [](uint64_t accumulator, const TextureMapDescriptor& tmap) { return accumulator + tmap.size; });

    // DXT compression if needed
    for(auto&& tmap: desc.texture_maps)
    {
        if(tmap.compression == TextureCompression::DXT5)
        {
            uint8_t* compressed = dxt::compress_DXT5(tmap.data, uint32_t(desc.width), uint32_t(desc.height));
            delete[] tmap.data;
            tmap.data = compressed;
        }
    }

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
			uint8_t(tmap.filter),
			tmap.channels,
            uint8_t(tmap.srgb),
			uint8_t(tmap.compression),
			tmap.size,
			uint64_t(tmap.name)
		};

		blocks.push_back(block);
	}

	// Generate header
	TOMHeader header;
	header.magic              = TOM_MAGIC;
	header.version_major      = TOM_VERSION_MAJOR;
	header.version_minor      = TOM_VERSION_MINOR;
	header.texture_width      = desc.width;
	header.texture_height     = desc.height;
	header.address_UV         = uint16_t(desc.address_UV);
	header.num_maps           = num_maps;
	header.blob_compression   = uint16_t(desc.compression);
	header.blob_size          = blob_size;
    header.blob_inflate_size  = blob_size;
    header.material_data_size = desc.material_data_size;
    header.material_type      = uint8_t(desc.material_type);

    auto ofs = wfs::get_ostream(desc.filepath, wfs::binary);

    // Compress blob if required
    if(desc.compression == LosslessCompression::Deflate)
    {
    	// DEFLATE compression
        uint32_t max_size = uint32_t(erwin::get_max_compressed_len(int(blob_size)));
        uint8_t* deflated = new uint8_t[max_size];
        header.blob_size  = uint32_t(erwin::compress_data(blob, int(blob_size), deflated, int(max_size)));
        header.blob_inflate_size = blob_size;

        // Free uncompressed blob buffer and reassign
        delete[] blob;
        blob = deflated;
    }

    // Write header
    ofs->write(opaque_cast(&header), sizeof(TOMHeader));
    // Write material data
    if(desc.material_data)
        ofs->write(opaque_cast(desc.material_data), long(header.material_data_size));
    // Write block descriptors
    ofs->write(opaque_cast(blocks.data()), num_maps*sizeof(BlockDescriptor));
    // Write blob
    ofs->write(opaque_cast(blob), long(header.blob_size));

    // Cleanup
    delete[] blob;
}

} // namespace tom
} // namespace erwin