#pragma once

#include "asset/loader_common.h"
#include "asset/mesh.h"
#include "filesystem/wesh_file.h"

namespace erwin
{

class MeshLoader
{
public:
    using Resource = Mesh;
    using DataDescriptor = wesh::WeshDescriptor;

    static AssetMetaData build_meta_data(const std::string& file_path);
    static DataDescriptor load_from_file(const AssetMetaData& meta_data);
    static Resource upload(const DataDescriptor& descriptor, hash_t resource_id);
    static void destroy(Resource& resource);
};

} // namespace erwin