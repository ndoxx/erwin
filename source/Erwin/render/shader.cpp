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


} // namespace erwin