#pragma once

#include "asset/loader_common.h"
#include "asset/texture.h"
#include "render/handles.h"
#include "render/texture_common.h"

#include <optional>

namespace erwin
{

class TextureLoader
{
public:
    using Resource = FreeTexture;
    using DataDescriptor = Texture2DDescriptor;

    static AssetMetaData build_meta_data(const fs::path& file_path);
    static DataDescriptor load_from_file(const AssetMetaData& meta_data, std::optional<DataDescriptor> options = {});
    static Resource upload(const DataDescriptor& descriptor);
    static void destroy(Resource& resource);
};

} // namespace erwin