#include "texture_loader.h"
#include "filesystem/image_file.h"
#include "render/renderer.h"
#include "utils/future.hpp"

namespace erwin
{
namespace experimental
{

AssetMetaData TextureLoader::build_meta_data(const fs::path& file_path)
{
    // Sanity check
    W_ASSERT(fs::exists(file_path), "File does not exist.");
    hash_t hextension = H_(file_path.extension().string().c_str());
    bool compatible = (hextension == ".hdr"_h) || (hextension == ".png"_h);
    W_ASSERT_FMT(compatible, "Incompatible file type: %s", file_path.extension().string().c_str());

    AssetMetaData::AssetType asset_type;
    switch(hextension)
    {
    case ".png"_h:
        asset_type = AssetMetaData::AssetType::ImageFilePNG;
        break;
    case ".hdr"_h:
        asset_type = AssetMetaData::AssetType::ImageFileHDR;
        break;
    }
    return {file_path, asset_type};
}

Texture2DDescriptor TextureLoader::load_from_file(const AssetMetaData& meta_data)
{
    Texture2DDescriptor descriptor;
    if(meta_data.type == AssetMetaData::AssetType::ImageFileHDR)
    {
        // Load HDR file
        img::HDRDescriptor hdrfile{meta_data.file_path};
        img::read_hdr(hdrfile);

        DLOGI << "Width:    " << WCC('v') << hdrfile.width << std::endl;
        DLOGI << "Height:   " << WCC('v') << hdrfile.height << std::endl;
        DLOGI << "Channels: " << WCC('v') << hdrfile.channels << std::endl;

        if(2 * hdrfile.height != hdrfile.width)
        {
            DLOGW("asset") << "HDR file must be in 2:1 format (width = 2 * height) for optimal results." << std::endl;
        }

        descriptor.width = hdrfile.width;
        descriptor.height = hdrfile.height;
        descriptor.mips = 0;
        descriptor.data = hdrfile.data;
        descriptor.image_format = ImageFormat::RGB32F;
        descriptor.flags = TF_MUST_FREE; // Let the renderer free the resources once the texture is loaded
    }
    else if(meta_data.type == AssetMetaData::AssetType::ImageFilePNG)
    {
        // Load PNG file
        img::PNGDescriptor pngfile{meta_data.file_path};
        // Force 4 channels
        pngfile.channels = 4;
        img::read_png(pngfile);

        DLOGI << "Width:    " << WCC('v') << pngfile.width << std::endl;
        DLOGI << "Height:   " << WCC('v') << pngfile.height << std::endl;
        DLOGI << "Channels: " << WCC('v') << pngfile.channels << std::endl;

        descriptor.width = pngfile.width;
        descriptor.height = pngfile.height;
        descriptor.data = pngfile.data;
        descriptor.flags = TF_MUST_FREE; // Let the renderer free the resources once the texture is loaded
    }
    return descriptor;
}

TextureHandle TextureLoader::upload(const Texture2DDescriptor& descriptor)
{
    return Renderer::create_texture_2D(descriptor);
}

std::pair<TextureHandle, Texture2DDescriptor> TextureLoader::load(const fs::path& file_path)
{
    W_PROFILE_FUNCTION()

    // Check cache first
    hash_t hname = H_(file_path.string().c_str());
    auto findit = managed_resources_.find(hname);
    if(findit == managed_resources_.end())
    {
        DLOG("asset", 1) << "Loading image as a texture:" << std::endl;
        DLOG("asset", 1) << WCC('p') << file_path << WCC(0) << std::endl;

        AssetMetaData meta_data = build_meta_data(file_path);
        auto descriptor = load_from_file(meta_data);
        auto handle = upload(descriptor);
        managed_resources_[hname] = {handle, descriptor};
        return managed_resources_[hname];
    }
    else
    {
        DLOG("asset", 1) << "Getting texture " << WCC('i') << "from cache" << WCC(0) << ":" << std::endl;
        DLOG("asset", 1) << WCC('p') << file_path << WCC(0) << std::endl;

        return findit->second;
    }
}

hash_t TextureLoader::load_async(const fs::path& file_path)
{
    W_PROFILE_FUNCTION()

    // Check cache first
    hash_t hname = H_(file_path.string().c_str());
    if(managed_resources_.find(hname) == managed_resources_.end())
    {
        DLOG("asset", 1) << "Loading image as a texture (async):" << std::endl;
        DLOG("asset", 1) << WCC('p') << file_path << WCC(0) << std::endl;

        auto&& [token, fut] = promises_.future_operation();
        AssetMetaData meta_data = build_meta_data(file_path);
        file_loading_tasks_.push_back(FileLoadingTask{token, meta_data});
        upload_tasks_.push_back(UploadTask{token, meta_data, std::move(fut)});
    }

    return hname;
}

void TextureLoader::on_ready(hash_t future_texture, std::function<void(TextureHandle, const Texture2DDescriptor&)> then)
{
    after_upload_tasks_.push_back(AfterUploadTask{future_texture, then});
}

void TextureLoader::release(hash_t hname)
{
    W_PROFILE_FUNCTION()

    auto findit = managed_resources_.find(hname);
    if(findit != managed_resources_.end())
	    Renderer::destroy(findit->second.first);
}

void TextureLoader::launch_async_tasks()
{
    // TMP: single thread loading all resources
    std::thread task([&]() {
        for(auto&& task : file_loading_tasks_)
        {
            Texture2DDescriptor descriptor = load_from_file(task.meta_data);
            promises_.fulfill(task.token, std::move(descriptor));
        }
        file_loading_tasks_.clear();
    });
    task.detach();
}

void TextureLoader::update()
{
    W_PROFILE_FUNCTION()

    for(auto it = upload_tasks_.begin(); it != upload_tasks_.end();)
    {
        auto&& task = *it;
        if(is_ready(task.future_desc))
        {
            auto&& descriptor = task.future_desc.get();

            hash_t hname = H_(task.meta_data.file_path.string().c_str());
            managed_resources_[hname] = {upload(descriptor), descriptor};
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
            task.init(findit->second.first, findit->second.second);
            after_upload_tasks_.erase(it);
        }
        else
            ++it;
    }
}

} // namespace experimental
} // namespace erwin