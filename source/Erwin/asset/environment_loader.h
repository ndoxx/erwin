#pragma once

#include "render/texture_common.h"
#include "asset/loader_common.h"
#include "asset/environment.h"

namespace erwin
{

class EnvironmentLoader
{
public:
	using Resource = Environment;
	using DataDescriptor = Texture2DDescriptor;

    static AssetMetaData build_meta_data(const fs::path& file_path);
    static DataDescriptor load_from_file(const AssetMetaData& meta_data);
    static Resource upload(const DataDescriptor& descriptor);
    static void destroy(Resource& resource);
    static Resource managed_resource(const Resource&, const DataDescriptor&);
};

} // namespace erwin