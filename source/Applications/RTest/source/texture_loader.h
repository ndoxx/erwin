#pragma once

#include <cstdint>
#include <future>

#include "core/core.h"
#include "render/handles.h"
#include "render/texture_common.h"
#include "utils/promise_storage.hpp"

#include "loader_common.h"

namespace erwin
{
namespace experimental
{

class TextureLoader
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
        std::future<Texture2DDescriptor> future_desc;
    };

    struct AfterUploadTask
    {
        hash_t name;
        std::function<void(TextureHandle, const Texture2DDescriptor&)> init;
    };

    std::pair<TextureHandle, Texture2DDescriptor> load(const fs::path& file_path);
    hash_t load_async(const fs::path& file_path);
    void on_ready(hash_t future_texture, std::function<void(TextureHandle, const Texture2DDescriptor&)> then);
    void release(hash_t texture_name);

    void launch_async_tasks();
    void update();

private:
    static AssetMetaData build_meta_data(const fs::path& file_path);
    static Texture2DDescriptor load_from_file(const AssetMetaData& meta_data);
    static TextureHandle upload(const Texture2DDescriptor& descriptor);

private:
    std::map<hash_t, std::pair<TextureHandle, Texture2DDescriptor>> managed_resources_;
    PromiseStorage<Texture2DDescriptor> promises_;
    std::vector<FileLoadingTask> file_loading_tasks_;
    std::vector<UploadTask> upload_tasks_;
    std::vector<AfterUploadTask> after_upload_tasks_;
};

} // namespace experimental
} // namespace erwin