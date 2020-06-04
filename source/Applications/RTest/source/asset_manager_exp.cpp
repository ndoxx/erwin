#include "asset_manager_exp.h"
#include "entity/component_PBR_material.h"
#include "filesystem/tom_file.h"
#include "utils/future.hpp"
#include "utils/promise_storage.hpp"
#include "core/intern_string.h"

#include "render/renderer.h"
#include "render/renderer_3d.h"

#include <chrono>
#include <thread>

namespace erwin
{
namespace experimental
{

struct AssetMetaData
{
    enum class AssetType : uint8_t
    {
        None,
        ImagePNG,
        ImageHDR,
        MaterialTOM,
        Mesh
    };

    fs::path file_path;
    AssetType type;
};

class MaterialFactory
{
public:
	struct MaterialFileLoadingTask
	{
	    uint64_t token;
	    AssetMetaData meta_data;
	};

	struct MaterialCreationTask
	{
	    uint64_t token;
	    AssetMetaData meta_data;
	    std::future<tom::TOMDescriptor> future_tom;
	};

	struct MaterialInitTask
	{
	    hash_t name;
	    std::function<void(const ComponentPBRMaterial&)> init;
	};

	const ComponentPBRMaterial& load_material(const fs::path& file_path);
	hash_t load_material_async(const fs::path& file_path);
	void on_material_ready(hash_t future_mat, std::function<void(const ComponentPBRMaterial&)> then);
	ComponentPBRMaterial create_material(const tom::TOMDescriptor& descriptor);
	void release(hash_t material_name);
	
	void launch_async_tasks();
	void update();

private:
	static ImageFormat select_image_format(uint8_t channels, TextureCompression compression, bool srgb);

private:
	// PBR Material data
    PromiseStorage<tom::TOMDescriptor> tom_promises_;
    std::map<hash_t, ComponentPBRMaterial> pbr_materials_;
    std::vector<MaterialFileLoadingTask> material_file_loading_tasks_;
    std::vector<MaterialCreationTask> material_creation_tasks_;
    std::vector<MaterialInitTask> material_init_tasks_;
};

hash_t MaterialFactory::load_material_async(const fs::path& file_path)
{
	W_PROFILE_FUNCTION()

	hash_t hname = H_(file_path.string().c_str());

	// * Sanity check
	W_ASSERT(fs::exists(file_path), "[AssetManager] File does not exist.");
	W_ASSERT(!file_path.extension().string().compare(".tom"), "[AssetManager] Invalid input file.");

	// * Check cache first
	if(pbr_materials_.find(hname) == pbr_materials_.end())
	{
	    auto&& [token, fut] = tom_promises_.future_operation();
	    AssetMetaData meta_data{file_path, AssetMetaData::AssetType::MaterialTOM};
	    material_file_loading_tasks_.push_back(MaterialFileLoadingTask{token, meta_data});
	    material_creation_tasks_.push_back(MaterialCreationTask{token, meta_data, std::move(fut)});
	}

    return hname;
}

void MaterialFactory::on_material_ready(hash_t future_mat, std::function<void(const ComponentPBRMaterial&)> then)
{
    material_init_tasks_.push_back(MaterialInitTask{future_mat, then});
}

ImageFormat MaterialFactory::select_image_format(uint8_t channels, TextureCompression compression, bool srgb)
{
    if(channels == 4)
    {
        switch(compression)
        {
        case TextureCompression::None:
            return srgb ? ImageFormat::SRGB_ALPHA : ImageFormat::RGBA8;
        case TextureCompression::DXT1:
            return srgb ? ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1 : ImageFormat::COMPRESSED_RGBA_S3TC_DXT1;
        case TextureCompression::DXT5:
            return srgb ? ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5 : ImageFormat::COMPRESSED_RGBA_S3TC_DXT5;
        }
    }
    else if(channels == 3)
    {
        switch(compression)
        {
        case TextureCompression::None:
            return srgb ? ImageFormat::SRGB8 : ImageFormat::RGB8;
        case TextureCompression::DXT1:
            return srgb ? ImageFormat::COMPRESSED_SRGB_S3TC_DXT1 : ImageFormat::COMPRESSED_RGB_S3TC_DXT1;
        default: {
            DLOGE("texture") << "Unsupported compression option, defaulting to RGB8." << std::endl;
            return ImageFormat::RGB8;
        }
        }
    }
    else
    {
        DLOGE("texture") << "Only 3 or 4 color channels supported, but got: " << int(channels) << std::endl;
        DLOGI << "Defaulting to RGBA8." << std::endl;
        return ImageFormat::RGBA8;
    }
}

ComponentPBRMaterial MaterialFactory::create_material(const tom::TOMDescriptor& descriptor)
{
	W_PROFILE_FUNCTION()

    TextureGroup tg;
    // Create and register all texture maps
    for(auto&& tmap : descriptor.texture_maps)
    {
        ImageFormat format = select_image_format(tmap.channels, tmap.compression, tmap.srgb);
        TextureHandle tex = Renderer::create_texture_2D(Texture2DDescriptor{
            descriptor.width, descriptor.height, 3, tmap.data, format, tmap.filter, descriptor.address_UV,
            TF_MUST_FREE}); // Let the renderer free the resources once the texture is loaded
        tg.textures[tg.texture_count++] = tex;
    }

#ifdef W_DEBUG
	DLOGI << "Found " << WCC('v') << tg.texture_count << WCC(0) << " texture maps. TextureHandles: { ";
	for(size_t ii=0; ii<tg.texture_count; ++ii)
	{
		DLOG("texture",1) << WCC('v') << tg.textures[ii].index << " ";
	}
	DLOG("texture",1) << WCC(0) << "}" << std::endl;
#endif

	W_ASSERT(descriptor.material_type == tom::MaterialType::PBR, "[AssetManager] Material is not PBR.");
	W_ASSERT(descriptor.material_data_size == sizeof(ComponentPBRMaterial::MaterialData), "[AssetManager] Invalid material data size.");
	
	ShaderHandle shader     = AssetManager::load_shader("shaders/deferred_PBR.glsl");
	UniformBufferHandle ubo = AssetManager::create_material_data_buffer<ComponentPBRMaterial>();
	
	std::string name = descriptor.filepath.stem().string();

	Material mat = {H_(name.c_str()), tg, shader, ubo, sizeof(ComponentPBRMaterial::MaterialData)};
	Renderer3D::register_shader(shader);
	Renderer::shader_attach_uniform_buffer(shader, ubo);

	ComponentPBRMaterial pbr_mat;
	pbr_mat.set_material(mat);
	memcpy(&pbr_mat.material_data, descriptor.material_data, descriptor.material_data_size);
	delete[] descriptor.material_data;

	return pbr_mat;
}

const ComponentPBRMaterial& MaterialFactory::load_material(const fs::path& file_path)
{
	W_PROFILE_FUNCTION()

	hash_t hname = H_(file_path.string().c_str());

	// * Sanity check
	W_ASSERT(fs::exists(file_path), "[AssetManager] File does not exist.");
	W_ASSERT(!file_path.extension().string().compare(".tom"), "[AssetManager] Invalid input file.");


	// * Check cache first
	auto findit = pbr_materials_.find(hname);
	if(findit != pbr_materials_.end())
	{
		DLOG("asset",1) << "[AssetManager] Getting PBR material " << WCC('i') << "from cache" << WCC(0) << ":" << std::endl;
		DLOG("asset",1) << WCC('p') << file_path << WCC(0) << std::endl;

		return findit->second;
	}
	else
	{
		DLOGN("asset") << "[AssetManager] Loading PBR material:" << std::endl;
		DLOG("asset",1) << WCC('p') << file_path << WCC(0) << std::endl;

	    tom::TOMDescriptor descriptor;
	    descriptor.filepath = file_path;
	    tom::read_tom(descriptor);

	    pbr_materials_[hname] = create_material(descriptor);

	    return pbr_materials_[hname];
	}
}

void MaterialFactory::release(hash_t hname)
{
	W_PROFILE_FUNCTION()

	auto it = pbr_materials_.find(hname);
	if(it!=pbr_materials_.end())
	{
		auto& mat = it->second;
		for(size_t ii=0; ii<mat.material.texture_group.texture_count; ++ii)
			Renderer::destroy(mat.material.texture_group[ii]);
	}
	pbr_materials_.erase(it);
}

void MaterialFactory::launch_async_tasks()
{
    // TMP: single thread loading all resources
    std::thread task([&]() {
        for(auto&& task : material_file_loading_tasks_)
        {
#if 0
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
#endif

			DLOGN("asset") << "[AssetManager] Loading PBR material (async):" << std::endl;
			DLOG("asset",1) << WCC('p') << task.meta_data.file_path << WCC(0) << std::endl;

            tom::TOMDescriptor descriptor;
            descriptor.filepath = task.meta_data.file_path;
            tom::read_tom(descriptor);

            tom_promises_.fulfill(task.token, std::move(descriptor));
        }
        material_file_loading_tasks_.clear();
    });
    task.detach();
}

void MaterialFactory::update()
{
	W_PROFILE_FUNCTION()

    for(auto it = material_creation_tasks_.begin(); it != material_creation_tasks_.end();)
    {
        auto&& task = *it;
        if(is_ready(task.future_tom))
        {
            auto&& descriptor = task.future_tom.get();

            hash_t hname = H_(task.meta_data.file_path.string().c_str());
            pbr_materials_[hname] = create_material(descriptor);
            material_creation_tasks_.erase(it);
        }
        else
            ++it;
    }

    for(auto it = material_init_tasks_.begin(); it != material_init_tasks_.end();)
    {
        auto&& task = *it;
        auto findit = pbr_materials_.find(task.name);
        if(findit != pbr_materials_.end())
        {
            const ComponentPBRMaterial& pbr_mat = findit->second;
            task.init(pbr_mat);
            material_init_tasks_.erase(it);
        }
        else
            ++it;
    }
}


class TextureFactory
{
public:
	std::pair<TextureHandle, Texture2DDescriptor> load_texture(const fs::path& filepath);
	hash_t load_texture_async(const fs::path& filepath);
	void on_texture_ready(hash_t future_texture, std::function<void(TextureHandle, const Texture2DDescriptor&)> then);

	void launch_async_tasks();
	void update();

private:
};

std::pair<TextureHandle, Texture2DDescriptor> TextureFactory::load_texture(const fs::path& filepath)
{

	return {};
}

hash_t TextureFactory::load_texture_async(const fs::path& filepath)
{

	return 0;
}

void TextureFactory::on_texture_ready(hash_t future_texture, std::function<void(TextureHandle, const Texture2DDescriptor&)> then)
{

}

void TextureFactory::launch_async_tasks()
{

}

void TextureFactory::update()
{

}


static struct
{
	MaterialFactory material_factory;
	TextureFactory texture_factory;

	std::map<hash_t, ShaderHandle> shader_cache;
	std::map<uint64_t, UniformBufferHandle> ubo_cache;
} s_storage;

UniformBufferHandle AssetManager::create_material_data_buffer(uint64_t component_id, uint32_t size)
{
	W_PROFILE_FUNCTION()

	auto it = s_storage.ubo_cache.find(component_id);
	if(it!=s_storage.ubo_cache.end())
		return it->second;

	auto handle = Renderer::create_uniform_buffer("material_data", nullptr, size, UsagePattern::Dynamic);
	s_storage.ubo_cache.insert({component_id, handle});
	return handle;
}

ShaderHandle AssetManager::load_shader(const fs::path& filepath, const std::string& name)
{
	W_PROFILE_FUNCTION()

	hash_t hname = H_(filepath.string().c_str());
	auto it = s_storage.shader_cache.find(hname);
	if(it!=s_storage.shader_cache.end())
		return it->second;

	DLOGN("asset") << "[AssetManager] Creating new shader:" << std::endl;

	// First, check if shader file exists in system assets
	fs::path fullpath;
	if(fs::exists(wfs::get_system_asset_dir() / filepath))
		fullpath = wfs::get_system_asset_dir() / filepath;
	else
		fullpath = wfs::get_asset_dir() / filepath;

	std::string shader_name = name.empty() ? filepath.stem().string() : name;
	ShaderHandle handle = Renderer::create_shader(fullpath, shader_name);
	DLOG("asset",1) << "ShaderHandle: " << WCC('v') << handle.index << std::endl;
	s_storage.shader_cache.insert({hname, handle});

	return handle;
}

const ComponentPBRMaterial& AssetManager::load_material(const fs::path& file_path)
{
	return s_storage.material_factory.load_material(file_path);
}

void AssetManager::release_material(hash_t hname)
{
	s_storage.material_factory.release(hname);
}

std::pair<TextureHandle, Texture2DDescriptor> AssetManager::load_texture(const fs::path& filepath)
{
	return s_storage.texture_factory.load_texture(filepath);
}

hash_t AssetManager::load_material_async(const fs::path& file_path)
{
	return s_storage.material_factory.load_material_async(file_path);
}

void AssetManager::on_material_ready(hash_t future_mat, std::function<void(const ComponentPBRMaterial&)> then)
{
	s_storage.material_factory.on_material_ready(future_mat, then);
}

hash_t AssetManager::load_texture_async(const fs::path& filepath)
{
	return s_storage.texture_factory.load_texture_async(filepath);
}

void AssetManager::on_texture_ready(hash_t future_texture, std::function<void(TextureHandle, const Texture2DDescriptor&)> then)
{
	s_storage.texture_factory.on_texture_ready(future_texture, then);
}



void AssetManager::launch_async_tasks()
{
	s_storage.material_factory.launch_async_tasks();
	s_storage.texture_factory.launch_async_tasks();
}

void AssetManager::update()
{
	s_storage.material_factory.update();
	s_storage.texture_factory.update();
}

} // namespace experimental
} // namespace erwin