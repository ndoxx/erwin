#include "render/shader.h"
#include "debug/logger.h"

#include "render/render_device.h"
#include "platform/ogl_shader.h"

namespace erwin
{

std::shared_ptr<Shader> Shader::create(const std::string& name, std::istream& source_stream)
{
	switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "Shader: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return std::make_shared<OGLShader>(name, source_stream);
    }
}

std::shared_ptr<Shader> Shader::create(const std::string& name, const std::string& source_string)
{
	switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "Shader: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return std::make_shared<OGLShader>(name, source_string);
    }
}

// -------------------------------------------------------------------------------------------------

void ShaderBank::add(std::shared_ptr<Shader> p_shader)
{
    bank_.insert(std::make_pair(H_(p_shader->get_name().c_str()), p_shader));
}

void ShaderBank::load(const std::string& name, std::istream& source_stream)
{
    bank_.insert(std::make_pair(H_(name.c_str()), Shader::create(name, source_stream)));
}

void ShaderBank::load(const std::string& name, const std::string& source_string)
{
    bank_.insert(std::make_pair(H_(name.c_str()), Shader::create(name, source_string)));
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


} // namespace erwin