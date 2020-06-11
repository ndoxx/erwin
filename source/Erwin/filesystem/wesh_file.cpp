#include "filesystem/wesh_file.h"
#include "filesystem/filesystem.h"
#include "debug/logger.h"
#include "core/core.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace wesh
{

//#pragma pack(push,1)
struct WESHHeader
{
    uint32_t magic;         // Magic number to check file format validity
    uint16_t version_major; // Version major number
    uint16_t version_minor; // Version minor number
    uint32_t vertex_size;   // Float count inside a vertex
    uint32_t vertex_count;  // Number of vertices
    uint32_t index_count;   // Number of indices
};
//#pragma pack(pop)

#define WESH_MAGIC 0x48534557 // ASCII(WESH)
#define WESH_VERSION_MAJOR 0
#define WESH_VERSION_MINOR 3


WeshDescriptor read(const fs::path& path)
{
    auto ifs = wfs::get_istream(path, wfs::binary);

    // Read header & sanity check
    WESHHeader header;
    ifs->read(opaque_cast(&header), sizeof(WESHHeader));

    W_ASSERT(header.magic == WESH_MAGIC, "Invalid WESH file: magic number mismatch.");
    W_ASSERT(header.version_major == WESH_VERSION_MAJOR, "Invalid WESH file: version (major) mismatch.");
    W_ASSERT(header.version_minor == WESH_VERSION_MINOR, "Invalid WESH file: version (minor) mismatch.");

    DLOG("asset",0) << "WESH Header:" << std::endl;
    DLOGI << "Version:   " << WCC('v') << int(header.version_major) << "." << int(header.version_minor) << std::endl;
    DLOGI << "Vtx size:  " << WCC('v') << header.vertex_size << std::endl;
    DLOGI << "Vtx count: " << WCC('v') << header.vertex_count << std::endl;
    DLOGI << "Idx count: " << WCC('v') << header.index_count << std::endl;

    WeshDescriptor descriptor;
    // Read mesh extent
    ifs->read(opaque_cast(&descriptor.extent.value), long(6*sizeof(float)));

    DLOG("asset",0) << "Extent:" << std::endl;
    DLOGI << "xmin: " << descriptor.extent.xmin() << std::endl;
    DLOGI << "xmax: " << descriptor.extent.xmax() << std::endl;
    DLOGI << "ymin: " << descriptor.extent.ymin() << std::endl;
    DLOGI << "ymax: " << descriptor.extent.ymax() << std::endl;
    DLOGI << "zmin: " << descriptor.extent.zmin() << std::endl;
    DLOGI << "zmax: " << descriptor.extent.zmax() << std::endl;

    // Read vertex and indes data
    size_t vdata_float_count = header.vertex_count * header.vertex_size;
	descriptor.vertex_data.resize(vdata_float_count);
	descriptor.index_data.resize(header.index_count);
    ifs->read(opaque_cast(descriptor.vertex_data.data()), long(vdata_float_count*sizeof(float)));
    ifs->read(opaque_cast(descriptor.index_data.data()), long(header.index_count*sizeof(uint32_t)));

    return descriptor;
}

} // namespace wesh
} // namespace erwin