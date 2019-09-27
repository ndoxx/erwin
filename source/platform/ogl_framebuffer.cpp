#include "platform/ogl_framebuffer.h"
#include "debug/logger.h"

namespace erwin
{


OGLFramebuffer::OGLFramebuffer(uint32_t width, uint32_t height, bool use_depth_texture):
width_(width),
height_(height),
has_depth_(use_depth_texture)
{
    DLOG("render",1) << "OpenGL " << WCC('i') << "Framebuffer" << WCC(0) << " created." << std::endl;
    DLOGI << "Width*Height: " << width_ << "*" << height_ << std::endl;
    DLOGI << "Has depth:    " << (has_depth_ ? "true" : "false") << std::endl;
}

OGLFramebuffer::~OGLFramebuffer()
{
    DLOG("render",1) << "OpenGL " << WCC('i') << "Framebuffer" << WCC(0) << " destroyed." << std::endl;
}

} // namespace erwin