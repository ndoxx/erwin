#pragma once

#include <filesystem>
namespace fs = std::filesystem;

namespace erwin
{

struct AssetMetaData
{
    enum class AssetType : uint8_t
    {
        None,
        ImageFilePNG,
        ImageFileHDR,
        MaterialTOM,
        Mesh
    };

    fs::path file_path;
    AssetType type;
};

} // namespace erwin