#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "../Erwin/core/core.h"
#include "ogl_context.h"
#include <kibble/logger/logger.h>



namespace erwin
{

OGLContext::OGLContext(void* window_handle) : window_handle_(window_handle) { init(); }

void OGLContext::init()
{
    GLFWwindow* window = static_cast<GLFWwindow*>(window_handle_);
    glfwMakeContextCurrent(window);
    int status = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    if(!status)
    {
        KLOGF("render") << "Unable to load GLAD!" << std::endl;
        fatal();
    }

    KLOG("render", 1) << "[OpenGL]" << std::endl;
    KLOGI << "Vendor:   " << glGetString(GL_VENDOR) << std::endl;
    KLOGI << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    KLOGI << "Version:  " << glGetString(GL_VERSION) << std::endl;
    /*
        KLOGI << "Extensions: " << std::endl;
        GLint n_ext;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n_ext);
        for(int ii=0; ii<n_ext; ++ii)
        {
            KLOGI << "    " << glGetStringi(GL_EXTENSIONS, ii) << std::endl;
        }
    */

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

void OGLContext::swap_buffers() const { glfwSwapBuffers(static_cast<GLFWwindow*>(window_handle_)); }

void OGLContext::make_current() const
{
    GLFWwindow* current_context = glfwGetCurrentContext();
    GLFWwindow* window_context = static_cast<GLFWwindow*>(window_handle_);

    if(current_context != window_context)
        glfwMakeContextCurrent(window_context);
}

} // namespace erwin