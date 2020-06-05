#pragma once

#include "loader_common.h"

namespace erwin
{

struct ComponentPBRMaterial;
namespace tom
{
struct  TOMDescriptor;
}

namespace experimental
{

class MaterialLoader
{
public:
	using Resource = ComponentPBRMaterial;
	using DataDescriptor = tom::TOMDescriptor;

    static AssetMetaData build_meta_data(const fs::path& file_path);
    static tom::TOMDescriptor load_from_file(const AssetMetaData& meta_data);
    static ComponentPBRMaterial upload(const tom::TOMDescriptor& descriptor);
    static void destroy(ComponentPBRMaterial& resource);
    static ComponentPBRMaterial managed_resource(const ComponentPBRMaterial&, const tom::TOMDescriptor&);
};

} // namespace experimental
} // namespace erwin