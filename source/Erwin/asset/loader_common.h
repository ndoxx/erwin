#pragma once

#include "filesystem/wpath.h"

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

    WPath file_path;
    AssetType type;
};

} // namespace erwin