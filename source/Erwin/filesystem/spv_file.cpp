#include "filesystem/spv_file.h"
#include "filesystem/filesystem.h"
#include "core/core.h"
#include "debug/logger.h"

#include <fstream>
#include <cstring>
#include <string>

namespace erwin
{
namespace spv
{

#define SPV_MAGIC 0x07230203

std::vector<ShaderStageDescriptor> parse_stages(const fs::path& path)
{
    std::string entrypointname[6];

    auto ifs = filesystem::binary_stream(path);

    // Parse SPIR-V data
    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t genmagnum = 0;
    uint32_t bound = 0;
    uint32_t reserved = 0;

    ifs.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));
    W_ASSERT(magic == SPV_MAGIC, "Invalid .spv file, wrong magic number");
    ifs.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));
    ifs.read(reinterpret_cast<char*>(&genmagnum), sizeof(uint32_t));
    ifs.read(reinterpret_cast<char*>(&bound), sizeof(uint32_t));
    ifs.read(reinterpret_cast<char*>(&reserved), sizeof(uint32_t));

    std::vector<ShaderStageDescriptor> descs;
    while(true)
    {
        long pos = ifs.tellg();
        uint32_t bytes;
        ifs.read(reinterpret_cast<char*>(&bytes), sizeof(uint32_t));
        uint16_t opcode = uint16_t(bytes & 0x0000FFFF); // Take low word
        uint16_t wcount = uint16_t((bytes & 0xFFFF0000) >> 16); // Take high word

        if(opcode == 15) // OpEntryPoint
        {
            uint32_t execution_model;
            ifs.read(reinterpret_cast<char*>(&execution_model), sizeof(uint32_t));
            // Vertex = 0, TessellationControl = 1, TessellationEvaluation = 2, 
            // Geometry = 3, Fragment = 4, GLCompute = 5,
            if(execution_model < 6)
            {
                ShaderStageDescriptor desc;
                desc.execution_model = slang::ExecutionModel(execution_model);
                uint32_t entry_point;
                ifs.read(reinterpret_cast<char*>(&entry_point), sizeof(uint32_t));
                char c;
                while(true)
                {
                    ifs.read(&c, 1);
                    if(c == '\0') break;
                    desc.entry_point += c;
                }
                descs.push_back(desc);
            }
        }

        ifs.seekg(pos + long(wcount*sizeof(uint32_t)));
        if(!ifs) break;
    }

    return descs;
}

} // namespace spv
} // namespace erwin