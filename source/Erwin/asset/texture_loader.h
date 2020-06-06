#pragma once

#include "render/handles.h"
#include "render/texture_common.h"

#include "asset/loader_common.h"

namespace erwin
{

class TextureLoader
{
public:
	using Resource = std::pair<TextureHandle, Texture2DDescriptor>;
	using DataDescriptor = Texture2DDescriptor;

    static AssetMetaData build_meta_data(const fs::path& file_path);
    static Texture2DDescriptor load_from_file(const AssetMetaData& meta_data);
    static TextureHandle upload(const Texture2DDescriptor& descriptor);
    static void destroy(Resource& resource);
    static Resource managed_resource(TextureHandle, const Texture2DDescriptor&);
};

} // namespace erwin