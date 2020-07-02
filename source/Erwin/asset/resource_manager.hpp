#pragma once

#include <filesystem>
#include <functional>
#include <future>
#include <map>
#include <mutex>

#include "asset/loader_common.h"
#include "core/core.h"
#include "filesystem/image_file.h"
#include "filesystem/tom_file.h"
#include "filesystem/file_path.h"
#include "utils/future.hpp"
#include "utils/promise_storage.hpp"

namespace fs = std::filesystem;

namespace erwin
{

template <typename LoaderT> class ResourceManager
{
public:
    using ManagedResource = typename LoaderT::Resource;
    using DataDescriptor = typename LoaderT::DataDescriptor;

    template <typename... ArgsT> std::pair<const ManagedResource&, const AssetMetaData&> load(const FilePath& file_path, ArgsT&&... args)
    {
        W_PROFILE_FUNCTION()

        // Check cache first
        hash_t hname = file_path.resource_id();
        auto findit = managed_resources_.find(hname);
        if(findit == managed_resources_.end())
        {
            AssetMetaData meta_data = LoaderT::build_meta_data(file_path);
            // Parameter pack allows to pass loading options to the loader, this is only useful for pure image loading
            // as image files don't encapsulate the relevant engine data
            auto descriptor = LoaderT::load_from_file(meta_data, std::forward<ArgsT>(args)...);
            managed_resources_[hname] = std::move(LoaderT::upload(descriptor, hname));
            meta_data_[hname] = std::move(meta_data);
            return {managed_resources_[hname], meta_data_[hname]};
        }
        else
            return {findit->second, meta_data_[hname]};
    }

    std::pair<hash_t, const AssetMetaData&> load_async(const FilePath& file_path);

    void on_ready(hash_t hname, std::function<void(const ManagedResource&)> then);
    void release(hash_t hname);

    void async_work();
    void sync_work();

    inline const AssetMetaData& get_meta_data(hash_t hname) const { return meta_data_.at(hname); }

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

    std::map<hash_t, ManagedResource> managed_resources_;
    std::map<hash_t, AssetMetaData> meta_data_;
    PromiseStorage<DataDescriptor> promises_;
    std::vector<FileLoadingTask> file_loading_tasks_;
    std::vector<UploadTask> upload_tasks_;
    std::multimap<hash_t, std::function<void(const ManagedResource&)>> on_ready_callbacks_;

    std::mutex mutex_;
};

template <typename LoaderT> std::pair<hash_t, const AssetMetaData&> ResourceManager<LoaderT>::load_async(const FilePath& file_path)
{
    W_PROFILE_FUNCTION()

    // Check cache first
    hash_t hname = file_path.resource_id();
    if(meta_data_.find(hname) == meta_data_.end())
    {
        auto&& [token, fut] = promises_.future_operation();
        AssetMetaData meta_data = LoaderT::build_meta_data(file_path);
        file_loading_tasks_.push_back(FileLoadingTask{token, meta_data});
        upload_tasks_.push_back(UploadTask{token, meta_data, std::move(fut)});
        meta_data_[hname] = std::move(meta_data);
    }

    return {hname, meta_data_[hname]};
}

template <typename LoaderT>
void ResourceManager<LoaderT>::on_ready(hash_t hname, std::function<void(const ManagedResource&)> then)
{
    on_ready_callbacks_.emplace(hname, then);
}

template <typename LoaderT> void ResourceManager<LoaderT>::release(hash_t hname)
{
    W_PROFILE_FUNCTION()

    auto findit = managed_resources_.find(hname);
    if(findit != managed_resources_.end())
    {
        LoaderT::destroy(findit->second);
        managed_resources_.erase(findit);
        meta_data_.erase(hname);
    }
}

template <typename LoaderT> void ResourceManager<LoaderT>::async_work()
{
    for(const auto& task : file_loading_tasks_)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        promises_.fulfill(task.token, LoaderT::load_from_file(task.meta_data));
    }
    const std::lock_guard<std::mutex> lock(mutex_);
    file_loading_tasks_.clear();
}

template <typename LoaderT> void ResourceManager<LoaderT>::sync_work()
{
    W_PROFILE_FUNCTION()

    for(auto it = upload_tasks_.begin(); it != upload_tasks_.end();)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        auto&& task = *it;
        if(is_ready(task.future_desc))
        {
            auto&& descriptor = task.future_desc.get();

            hash_t hname = task.meta_data.file_path.resource_id();
            managed_resources_[hname] = std::move(LoaderT::upload(descriptor, hname));
            upload_tasks_.erase(it);

            // Call user callbacks
            auto range = on_ready_callbacks_.equal_range(hname);
            for(auto callback_it = range.first; callback_it != range.second; ++callback_it)
                callback_it->second(managed_resources_.at(hname));

            on_ready_callbacks_.erase(hname);
        }
        else
            ++it;
    }
}

} // namespace erwin