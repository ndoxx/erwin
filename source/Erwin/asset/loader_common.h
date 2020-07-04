#pragma once

#include "filesystem/file_path.h"

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

    FilePath file_path;
    AssetType type;
};

} // namespace erwin