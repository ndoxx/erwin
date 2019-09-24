#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "ogl_context.h"
#include "../Erwin/debug/logger.h"
#include "../Erwin/core/core.h"

namespace erwin
{

OGLContext::OGLContext(void* window_handle):
window_handle_(window_handle)
{
	init();
}

OGLContext::~OGLContext()
{

}

void OGLContext::init()
{
	GLFWwindow* window = static_cast<GLFWwindow*>(window_handle_);
	glfwMakeContextCurrent(window);
	int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	if(!status)
	{
		DLOGF("core") << "Unable to load GLAD!" << std::endl;
		fatal();
	}

	DLOG("core", 1) << "[OpenGL]" << std::endl;
	DLOGI << "Vendor:   " << glGetString(GL_VENDOR) << std::endl;
	DLOGI << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	DLOGI << "Version:  " << glGetString(GL_VERSION) << std::endl;
/*
	DLOGI << "Extensions: " << std::endl;
	GLint n_ext;
	glGetIntegerv(GL_NUM_EXTENSIONS, &n_ext);
	for(int ii=0; ii<n_ext; ++ii)
	{
		DLOGI << "    " << glGetStringi(GL_EXTENSIONS, ii) << std::endl;
	}
*/

	DLOGI << "Compression:" << std::endl;

	GLint n_comp;
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &n_comp);
	GLint* comp_list = new GLint[n_comp];
	glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, comp_list);
	for(int ii=0; ii<n_comp; ++ii)
	{
		switch(comp_list[ii])
		{
			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			{
				DLOGI << "    GL_COMPRESSED_RGB_S3TC_DXT1_EXT" << std::endl;
				break;
			}
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			{
				DLOGI << "    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT" << std::endl;
				break;
			}
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			{
				DLOGI << "    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT" << std::endl;
				break;
			}
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			{
				DLOGI << "    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT" << std::endl;
				break;
			}
		}
	}

	// Pop any GL error that would have been caused by say... GLFW init
	glGetError();
}

void OGLContext::swap_buffers()
{
	glfwSwapBuffers(static_cast<GLFWwindow*>(window_handle_));
}


} // namespace erwin