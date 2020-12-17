#include "filesystem/wesh_file.h"
#include "core/core.h"
#include "filesystem/filesystem.h"
#include <kibble/logger/logger.h>

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

WeshDescriptor read(const WPath& path)
{
    auto ifs = wfs::get_istream(path, wfs::binary);

    // Read header & sanity check
    WESHHeader header;
    ifs->read(opaque_cast(&header), sizeof(WESHHeader));

    K_ASSERT(header.magic == WESH_MAGIC, "Invalid WESH file: magic number mismatch.");
    K_ASSERT(header.version_major == WESH_VERSION_MAJOR, "Invalid WESH file: version (major) mismatch.");
    K_ASSERT(header.version_minor == WESH_VERSION_MINOR, "Invalid WESH file: version (minor) mismatch.");

    KLOG("asset", 0) << "WESH Header:" << std::endl;
    KLOGI << "Version:   " << kb::KS_VALU_ << int(header.version_major) << "." << int(header.version_minor) << std::endl;
    KLOGI << "Vtx size:  " << kb::KS_VALU_ << header.vertex_size << std::endl;
    KLOGI << "Vtx count: " << kb::KS_VALU_ << header.vertex_count << std::endl;
    KLOGI << "Idx count: " << kb::KS_VALU_ << header.index_count << std::endl;

    WeshDescriptor descriptor;
    // Read mesh extent
    ifs->read(opaque_cast(&descriptor.extent.value), long(6 * sizeof(float)));

    KLOG("asset", 0) << "Extent:" << std::endl;
    KLOGI << "xmin: " << descriptor.extent.xmin() << std::endl;
    KLOGI << "xmax: " << descriptor.extent.xmax() << std::endl;
    KLOGI << "ymin: " << descriptor.extent.ymin() << std::endl;
    KLOGI << "ymax: " << descriptor.extent.ymax() << std::endl;
    KLOGI << "zmin: " << descriptor.extent.zmin() << std::endl;
    KLOGI << "zmax: " << descriptor.extent.zmax() << std::endl;

    // Read vertex and indes data
    size_t vdata_float_count = header.vertex_count * header.vertex_size;
    descriptor.vertex_data.resize(vdata_float_count);
    descriptor.index_data.resize(header.index_count);
    ifs->read(opaque_cast(descriptor.vertex_data.data()), long(vdata_float_count * sizeof(float)));
    ifs->read(opaque_cast(descriptor.index_data.data()), long(header.index_count * sizeof(uint32_t)));

    return descriptor;
}

} // namespace wesh
} // namespace erwin