#pragma once

#include <string>
#include <cstdint>

namespace erwin
{

struct AssetMetaData
{
    enum class AssetType : uint8_t
    {
        None,
        ImageFilePNG,
        ImageFileHDR,
        EnvironmentHDR,
        MaterialTOM,
        TextureAtlasCAT,
        FontAtlasCAT,
        MeshWESH
    };

    std::string file_path;
    AssetType type;
};

} // namespace erwin