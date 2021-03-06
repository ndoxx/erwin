#include "filesystem/spv_file.h"
#include "core/application.h"
#include <kibble/logger/logger.h>

#include <cstring>
#include <string>

namespace erwin
{
namespace spv
{

// Helpers for stream read/write pointer cast
// Only well defined for PODs
template <typename T, typename = std::enable_if_t<std::is_standard_layout_v<T> && std::is_trivial_v<T>>>
static inline char* opaque_cast(T* in)
{
    return reinterpret_cast<char*>(in);
}

template <typename T, typename = std::enable_if_t<std::is_standard_layout_v<T> && std::is_trivial_v<T>>>
static inline const char* opaque_cast(const T* in)
{
    return reinterpret_cast<const char*>(in);
}

#define SPV_MAGIC 0x07230203

std::vector<ShaderStageDescriptor> parse_stages(const std::string& path)
{
    std::string entrypointname[6];

    auto ifs = WFS_.get_input_stream(path);

    // Parse SPIR-V data
    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t genmagnum = 0;
    uint32_t bound = 0;
    uint32_t reserved = 0;

    ifs->read(opaque_cast(&magic), sizeof(uint32_t));
    K_ASSERT(magic == SPV_MAGIC, "Invalid .spv file, wrong magic number");
    ifs->read(opaque_cast(&version), sizeof(uint32_t));
    ifs->read(opaque_cast(&genmagnum), sizeof(uint32_t));
    ifs->read(opaque_cast(&bound), sizeof(uint32_t));
    ifs->read(opaque_cast(&reserved), sizeof(uint32_t));

    std::vector<ShaderStageDescriptor> descs;
    while(true)
    {
        long pos = ifs->tellg();
        uint32_t bytes;
        ifs->read(opaque_cast(&bytes), sizeof(uint32_t));
        uint16_t opcode = uint16_t(bytes & 0x0000FFFF);         // Take low word
        uint16_t wcount = uint16_t((bytes & 0xFFFF0000) >> 16); // Take high word

        if(opcode == 15) // OpEntryPoint
        {
            uint32_t execution_model;
            ifs->read(opaque_cast(&execution_model), sizeof(uint32_t));
            // Vertex = 0, TessellationControl = 1, TessellationEvaluation = 2,
            // Geometry = 3, Fragment = 4, GLCompute = 5,
            if(execution_model < 6)
            {
                ShaderStageDescriptor desc;
                desc.execution_model = slang::ExecutionModel(execution_model);
                uint32_t entry_point;
                ifs->read(opaque_cast(&entry_point), sizeof(uint32_t));
                char c;
                while(true)
                {
                    ifs->read(&c, 1);
                    if(c == '\0')
                        break;
                    desc.entry_point += c;
                }
                descs.push_back(desc);
            }
        }

        ifs->seekg(pos + long(wcount * sizeof(uint32_t)));
        if(!(*ifs))
            break;
    }

    return descs;
}

} // namespace spv
} // namespace erwin