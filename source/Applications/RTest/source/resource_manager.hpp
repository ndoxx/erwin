#pragma once

#include <filesystem>
#include <functional>
#include <future>

#include "core/core.h"
#include "filesystem/image_file.h"
#include "filesystem/tom_file.h"
#include "utils/promise_storage.hpp"
#include "utils/future.hpp"
#include "loader_common.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace experimental
{

template <typename LoaderT>
class ResourceManager
{
public:
	using ManagedResource = typename LoaderT::Resource;
	using DataDescriptor  = typename LoaderT::DataDescriptor;

    const ManagedResource& load(const fs::path& file_path);
    hash_t load_async(const fs::path& file_path);
    void on_ready(hash_t hname, std::function<void(const ManagedResource&)> then);
    void release(hash_t hname);

    void launch_async_tasks();
    void update();

private:
    struct FileLoadingTask
    {
        uint64_t token;
        AssetMetaData meta_data;
    };

    struct UploadTask
    {
        uint64_t token;
        AssetMetaData meta_data;
        std::future<DataDescriptor> future_desc;
    };

    struct AfterUploadTask
    {
        hash_t name;
        std::function<void(const ManagedResource&)> init;
    };

    std::map<hash_t, ManagedResource> managed_resources_;
    PromiseStorage<DataDescriptor> promises_;
    std::vector<FileLoadingTask> file_loading_tasks_;
    std::vector<UploadTask> upload_tasks_;
    std::vector<AfterUploadTask> after_upload_tasks_;
};

template <typename LoaderT>
const typename ResourceManager<LoaderT>::ManagedResource& ResourceManager<LoaderT>::load(const fs::path& file_path)
{
    W_PROFILE_FUNCTION()

    // Check cache first
    hash_t hname = H_(file_path.string().c_str());
    auto findit = managed_resources_.find(hname);
    if(findit == managed_resources_.end())
    {
        AssetMetaData meta_data = LoaderT::build_meta_data(file_path);
        auto descriptor = LoaderT::load_from_file(meta_data);
        managed_resources_[hname] = std::move(LoaderT::managed_resource(LoaderT::upload(descriptor), descriptor));
        return managed_resources_[hname];
    }
    else
        return findit->second;
}

template <typename LoaderT>
hash_t ResourceManager<LoaderT>::load_async(const fs::path& file_path)
{
    W_PROFILE_FUNCTION()

    // Check cache first
    hash_t hname = H_(file_path.string().c_str());
    if(managed_resources_.find(hname) == managed_resources_.end())
    {
        auto&& [token, fut] = promises_.future_operation();
        AssetMetaData meta_data = LoaderT::build_meta_data(file_path);
        file_loading_tasks_.push_back(FileLoadingTask{token, meta_data});
        upload_tasks_.push_back(UploadTask{token, meta_data, std::move(fut)});
    }

    return hname;
}

template <typename LoaderT>
void ResourceManager<LoaderT>::on_ready(hash_t hname, std::function<void(const ManagedResource&)> then)
{
    after_upload_tasks_.push_back(AfterUploadTask{hname, then});
}

template <typename LoaderT>
void ResourceManager<LoaderT>::release(hash_t hname)
{
    W_PROFILE_FUNCTION()

    auto findit = managed_resources_.find(hname);
    if(findit != managed_resources_.end())
	    LoaderT::destroy(findit->second);
}

template <typename LoaderT>
void ResourceManager<LoaderT>::launch_async_tasks()
{
    // TMP: single thread loading all resources
    std::thread task([&]() {
        for(auto&& task : file_loading_tasks_)
        {
            auto descriptor = LoaderT::load_from_file(task.meta_data);
            promises_.fulfill(task.token, std::move(descriptor));
        }
        file_loading_tasks_.clear();
    });
    task.detach();
}

template <typename LoaderT>
void ResourceManager<LoaderT>::update()
{
    W_PROFILE_FUNCTION()

    for(auto it = upload_tasks_.begin(); it != upload_tasks_.end();)
    {
        auto&& task = *it;
        if(is_ready(task.future_desc))
        {
            auto&& descriptor = task.future_desc.get();

            hash_t hname = H_(task.meta_data.file_path.string().c_str());
            managed_resources_[hname] = std::move(LoaderT::managed_resource(LoaderT::upload(descriptor), descriptor));
            upload_tasks_.erase(it);
        }
        else
            ++it;
    }

    for(auto it = after_upload_tasks_.begin(); it != after_upload_tasks_.end();)
    {
        auto&& task = *it;
        auto findit = managed_resources_.find(task.name);
        if(findit != managed_resources_.end())
        {
            task.init(findit->second);
            after_upload_tasks_.erase(it);
        }
        else
            ++it;
    }
}

} // namespace experimental
} // namespace erwin