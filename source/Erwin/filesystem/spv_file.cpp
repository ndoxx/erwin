#include "filesystem/spv_file.h"
#include "core/core.h"

#include <fstream>
#include <cstring>

namespace erwin
{
namespace spv
{

// SPV file format
//#pragma pack(push,1)
struct SPVHeader
{
    uint32_t magic;         // Magic number to check file format validity
    uint16_t version_major; // Version major number
    uint16_t version_minor; // Version minor number
    uint32_t num_shaders;   // Number of shaders inside program
};
//#pragma pack(pop)
#define SPV_MAGIC 0x56505357 // ASCII(WSPV)
#define SPV_VERSION_MAJOR 1
#define SPV_VERSION_MINOR 0

void read_spv(SPVDescriptor& desc)
{

}

void write_spv(const SPVDescriptor& desc)
{
    SPVHeader header;
    header.magic = SPV_MAGIC;
    header.version_major = SPV_VERSION_MAJOR;
    header.version_minor = SPV_VERSION_MINOR;
    header.num_shaders = desc.shaders.size();

    std::ofstream ofs(desc.filepath, std::ios::binary);
    // Write header
    ofs.write(reinterpret_cast<const char*>(&header), sizeof(SPVHeader));
    // Write shader properties and data
    for(auto&& shd: desc.shaders)
    {
        uint32_t size = shd.data.size();
        char entry_point[32];
        strncpy(entry_point, shd.entry_point.data(), shd.entry_point.size());
        ofs.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
        ofs.write(reinterpret_cast<const char*>(&shd.type), sizeof(uint8_t));
        ofs.write(reinterpret_cast<const char*>(entry_point), 32);
        ofs.write(reinterpret_cast<const char*>(shd.data.data()), shd.data.size());
    }
}



} // namespace spv
} // namespace erwin