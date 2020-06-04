#pragma once

#include <cstdint>
#include <future>

#include "core/core.h"
#include "filesystem/tom_file.h"
#include "render/texture_common.h"
#include "utils/promise_storage.hpp"

#include "loader_common.h"

namespace erwin
{
struct ComponentPBRMaterial;

namespace experimental
{

class MaterialLoader
{
public:
    struct FileLoadingTask
    {
        uint64_t token;
        AssetMetaData meta_data;
    };

    struct UploadTask
    {
        uint64_t token;
        AssetMetaData meta_data;
        std::future<tom::TOMDescriptor> future_tom;
    };

    struct AfterUploadTask
    {
        hash_t name;
        std::function<void(const ComponentPBRMaterial&)> init;
    };

    const ComponentPBRMaterial& load(const fs::path& file_path);
    hash_t load_async(const fs::path& file_path);
    void on_ready(hash_t future_mat, std::function<void(const ComponentPBRMaterial&)> then);
    void release(hash_t material_name);

    void launch_async_tasks();
    void update();

private:
    static AssetMetaData build_meta_data(const fs::path& file_path);
    static tom::TOMDescriptor load_from_file(const AssetMetaData& meta_data);
    static ComponentPBRMaterial upload(const tom::TOMDescriptor& descriptor);

private:
    // PBR Material data
    std::map<hash_t, ComponentPBRMaterial> managed_resources_;
    PromiseStorage<tom::TOMDescriptor> promises_;
    std::vector<FileLoadingTask> file_loading_tasks_;
    std::vector<UploadTask> upload_tasks_;
    std::vector<AfterUploadTask> after_upload_tasks_;
};

} // namespace experimental
} // namespace erwin