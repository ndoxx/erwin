#include "render/shader.h"
#include "debug/logger.h"

#include "render/render_device.h"
#include "platform/ogl_shader.h"

namespace erwin
{

void ShaderParameters::set_texture_slot(hash_t sampler_name, WRef<Texture2D> texture)
{
    texture_slots.insert(std::make_pair(sampler_name, texture));
}

WRef<Shader> Shader::create(const std::string& name, const fs::path& filepath)
{
	switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "Shader: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_ref<OGLShader>(name, filepath);
    }
}

WRef<Shader> Shader::create(const std::string& name, const std::string& source_string)
{
	switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "Shader: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_ref<OGLShader>(name, source_string);
    }
}

// -------------------------------------------------------------------------------------------------

void ShaderBank::add(WRef<Shader> p_shader)
{
    hash_t hname = H_(p_shader->get_name().c_str());
    if(exists(hname))
    {
        DLOGW("shader") << "Shader " << p_shader->get_name() << " already loaded." << std::endl;
        return;
    }
    bank_.insert(std::make_pair(hname, p_shader));
    next_index(hname);
}

void ShaderBank::load(const fs::path& path)
{
    /*auto stream = filesystem::get_asset_stream(path);
    load(path.stem().string(), stream);*/
    fs::path shader_path = filesystem::get_asset_dir() / path;
    if(!fs::exists(shader_path))
    {
        DLOGE("shader") << "Unable to find file: " << WCC('p') << shader_path << std::endl;
        return;
    }

    std::string name = path.stem().string();
    hash_t hname = H_(name.c_str());
    bank_.insert(std::make_pair(hname, Shader::create(name, shader_path)));
    next_index(hname);
}

void ShaderBank::load(const std::string& name, const std::string& source_string)
{
    hash_t hname = H_(name.c_str());
    if(exists(hname))
    {
        DLOGW("shader") << "Shader " << name << " already loaded." << std::endl;
        return;
    }
    bank_.insert(std::make_pair(hname, Shader::create(name, source_string)));
    next_index(hname);
}

const Shader& ShaderBank::get(hash_t name) const
{
    auto it = bank_.find(name);
    W_ASSERT(it!=bank_.end(), "ShaderBank: shader could not be found!");
    return *(it->second);
}

bool ShaderBank::exists(hash_t name) const
{
    return (bank_.find(name) != bank_.end());
}

void ShaderBank::next_index(hash_t hname)
{
    indices_.insert(std::make_pair(hname, ++current_index_));
}


} // namespace erwin