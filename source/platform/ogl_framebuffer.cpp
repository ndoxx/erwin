#include "platform/ogl_framebuffer.h"
#include "platform/ogl_texture.h"
#include "debug/logger.h"

#include "stb/stb_image_write.h"

#include "glad/glad.h"

namespace erwin
{


OGLFramebuffer::OGLFramebuffer(uint32_t width, uint32_t height, const FramebufferLayout& layout, bool depth, bool stencil):
Framebuffer(width, height, layout, depth, stencil)
{
    DLOG("render",1) << "Creating OpenGL " << WCC('i') << "Framebuffer" << WCC(0) << "." << std::endl;

    glCreateFramebuffers(1, &rd_handle_);
    glBindFramebuffer(GL_FRAMEBUFFER, rd_handle_);

    DLOGI << "handle:       " << rd_handle_ << std::endl;
    DLOGI << "width*height: " << width_ << "*" << height_ << std::endl;
    DLOGI << "#color bufs:  " << layout.get_count() << std::endl;
    DLOGI << "has depth:    " << (has_depth_attachment_ ? "true" : "false") << std::endl;
    DLOGI << "has stencil:  " << (has_stencil_attachment_ ? "true" : "false") << std::endl;

    // * Color buffers
    std::vector<GLenum> draw_buffers;
    int ncolor_attachments = 0;
    for(auto&& elt: layout_)
    {
    	// First, create textures for color buffers
    	auto texture = Texture2D::create(Texture2DDescriptor{width_, height_, nullptr, elt.image_format, elt.filter, elt.wrap, false});

    	// Register color attachment
    	uint32_t texture_handle = std::static_pointer_cast<OGLTexture2D>(texture)->get_handle();
    	GLenum attachment = GLenum(int(GL_COLOR_ATTACHMENT0) + ncolor_attachments++);
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture_handle, 0);
        // Save texture and attachments
    	textures_.push_back(texture);
        draw_buffers.push_back(attachment);
    }
    // Specify list of color buffers to draw to
    if(has_depth_attachment_ && ncolor_attachments == 0) // Depth only texture
        glDrawBuffer(GL_NONE);
    else
        glDrawBuffers(ncolor_attachments, draw_buffers.data());


    // * Handle depth / depth-stencil texture and attachment creation
    if(has_depth_attachment_ && has_stencil_attachment_)
    {
    	auto texture = Texture2D::create(Texture2DDescriptor{width_, height_, nullptr, ImageFormat::DEPTH24_STENCIL8});
    	uint32_t texture_handle = std::static_pointer_cast<OGLTexture2D>(texture)->get_handle();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture_handle, 0);
    	textures_.push_back(texture);
    }
    else if(has_depth_attachment_)
    {
    	auto texture = Texture2D::create(Texture2DDescriptor{width_, height_, nullptr, ImageFormat::DEPTH_COMPONENT24});
    	uint32_t texture_handle = std::static_pointer_cast<OGLTexture2D>(texture)->get_handle();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_handle, 0);
    	textures_.push_back(texture);
    }

    // No depth texture -> Attach a render buffer to frame buffer as a z-buffer
    // or else, framebuffer is not complete
    if(!has_depth_attachment_)
    {
        glCreateRenderbuffers(1, &render_buffer_handle_);
        glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_handle_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer_handle_);
    }

    framebuffer_error_report();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

OGLFramebuffer::~OGLFramebuffer()
{
    DLOG("render",1) << "OpenGL " << WCC('i') << "Framebuffer" << WCC(0) << " [" << rd_handle_ << "] destroyed." << std::endl;
    if(rd_handle_) glDeleteFramebuffers(1, &rd_handle_);
    if(render_buffer_handle_) glDeleteRenderbuffers(1, &render_buffer_handle_);
}

void OGLFramebuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, rd_handle_);
    glViewport(0, 0, width_, height_);
}

void OGLFramebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

WRef<Texture> OGLFramebuffer::get_shared_texture(uint32_t index)
{
    return textures_[index];
}

uint32_t OGLFramebuffer::get_texture_count()
{
    return uint32_t(textures_.size());
}

void OGLFramebuffer::screenshot(const std::string& filepath)
{
    DLOGN("render") << "[Framebuffer] Saving screenshot:" << std::endl;

    // Allocate buffer for image data
    unsigned char* pixels = new unsigned char[width_ * height_ * 4];

    // Retrieve pixel data
    glFinish();
    glBindFramebuffer(GL_FRAMEBUFFER, rd_handle_);
    glPixelStorei(GL_PACK_ALIGNMENT, 1); // Change alignment to 1 to avoid out of bounds writes,
    glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Write to png file
    stbi_write_png(filepath.c_str(), int(width_), int(height_), 4, pixels, int(width_) * 4);

    // Cleanup
    delete[] pixels;

    DLOGI << WCC('p') << filepath << std::endl;
}

void OGLFramebuffer::blit_depth(const Framebuffer& source)
{
    // Push state
    GLint draw_fbo = 0, read_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fbo);

    const OGLFramebuffer& ogl_source = static_cast<const OGLFramebuffer&>(source);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, ogl_source.rd_handle_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rd_handle_);
    glBlitFramebuffer(0,                    // src x0
                      0,                    // src y0
                      ogl_source.width_,    // src x1
                      ogl_source.height_,   // src y1
                      0,                    // dst x0
                      0,                    // dst y0
                      width_,               // dst x1
                      height_,              // dst y1
                      GL_DEPTH_BUFFER_BIT,  // mask
                      GL_NEAREST);          // filter

    // Pop state
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fbo);
}

void OGLFramebuffer::framebuffer_error_report()
{
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        switch(status)
        {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                DLOGE("render") << "[Framebuffer] Not all framebuffer attachment points are framebuffer attachment complete." << std::endl;
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                DLOGE("render") << "[Framebuffer] No images are attached to the framebuffer." << std::endl;
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                DLOGE("render") << "[Framebuffer] The combination of internal formats of the attached images violates an implementation-dependent set of restrictions." << std::endl;
                break;
        }
    }
    else
        DLOG("render",1) << "Framebuffer [" << rd_handle_ << "] creation " << WCC('g') << "complete" << WCC(0) << "." << std::endl;
}

} // namespace erwin