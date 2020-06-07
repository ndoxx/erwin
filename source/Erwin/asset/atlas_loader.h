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
    static cat::CATDescriptor load_from_file(const AssetMetaData& meta_data);
    static TextureAtlas upload(const cat::CATDescriptor& descriptor);
    static void destroy(TextureAtlas& resource);
    static TextureAtlas managed_resource(const TextureAtlas&, const cat::CATDescriptor&);
};

class FontAtlasLoader
{
public:
	using Resource = FontAtlas;
	using DataDescriptor = cat::CATDescriptor;

    static AssetMetaData build_meta_data(const fs::path& file_path);
    static cat::CATDescriptor load_from_file(const AssetMetaData& meta_data);
    static FontAtlas upload(const cat::CATDescriptor& descriptor);
    static void destroy(FontAtlas& resource);
    static FontAtlas managed_resource(const FontAtlas&, const cat::CATDescriptor&);
};

} // namespace erwin