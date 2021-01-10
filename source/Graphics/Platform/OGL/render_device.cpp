#include "Platform/OGL/render_device.h"
#include "../../window.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <kibble/logger/logger.h>

namespace gfx
{

OGLRenderDevice::OGLRenderDevice(const Window& window)
{
    window.make_current();

    int status = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
    if(!status)
    {
        KLOGF("render") << "Unable to load GLAD!" << std::endl;
        exit(0);
    }

    KLOG("render", 1) << "[OpenGL]" << std::endl;
    KLOGI << "Vendor:   " << glGetString(GL_VENDOR) << std::endl;
    KLOGI << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    KLOGI << "Version:  " << glGetString(GL_VERSION) << std::endl;

    KLOGI << "Compression:" << std::endl;

    GLint n_comp;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &n_comp);
    GLint* comp_list = new GLint[size_t(n_comp)];
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, comp_list);
    for(int ii = 0; ii < n_comp; ++ii)
    {
        switch(comp_list[ii])
        {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: {
            KLOGI << "    GL_COMPRESSED_RGB_S3TC_DXT1_EXT" << std::endl;
            break;
        }
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: {
            KLOGI << "    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT" << std::endl;
            break;
        }
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: {
            KLOGI << "    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT" << std::endl;
            break;
        }
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: {
            KLOGI << "    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT" << std::endl;
            break;
        }
        }
    }

    // Pop any GL error that would have been caused by say... GLFW init
    glGetError();
}

} // namespace gfx