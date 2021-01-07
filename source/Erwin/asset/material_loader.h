#pragma once

#include "core/core.h"
#include "asset/loader_common.h"

namespace erwin
{

struct ComponentPBRMaterial;
namespace tom
{
struct TOMDescriptor;
}

class MaterialLoader
{
public:
    using Resource = ComponentPBRMaterial;
    using DataDescriptor = tom::TOMDescriptor;

    static AssetMetaData build_meta_data(const std::string& file_path);
    static DataDescriptor load_from_file(const AssetMetaData& meta_data);
    static Resource upload(const DataDescriptor& descriptor, hash_t resource_id);
    static void destroy(Resource& resource);
};

} // namespace erwin