#pragma once

#include "asset/loader_common.h"
#include "asset/texture_atlas.h"
#include "filesystem/cat_file.h"

namespace erwin
{

class TextureAtlasLoader
{
public:
    using Resource = TextureAtlas;
    using DataDescriptor = cat::CATDescriptor;

    static AssetMetaData build_meta_data(const fs::path& file_path);
    static DataDescriptor load_from_file(const AssetMetaData& meta_data);
    static Resource upload(const DataDescriptor& descriptor);
    static void destroy(Resource& resource);
};

class FontAtlasLoader
{
public:
    using Resource = FontAtlas;
    using DataDescriptor = cat::CATDescriptor;

    static AssetMetaData build_meta_data(const fs::path& file_path);
    static DataDescriptor load_from_file(const AssetMetaData& meta_data);
    static Resource upload(const DataDescriptor& descriptor);
    static void destroy(Resource& resource);
};

} // namespace erwin