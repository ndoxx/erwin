#include "platform/OGL/ogl_framebuffer.h"
#include "platform/OGL/ogl_texture.h"
#include "platform/OGL/ogl_backend.h"
#include "debug/logger.h"

#include "stb/stb_image_write.h"

#include "glad/glad.h"

namespace erwin
{

OGLFramebuffer::OGLFramebuffer(uint32_t width, uint32_t height, uint8_t flags, const FramebufferLayout& layout, const FramebufferTextureVector& texture_vector)
    : layout_(layout), width_(width), height_(height), flags_(flags)
{
    if(has_cubemap())
    {
        W_ASSERT(!has_depth() && !has_stencil(),
                 "Cubemap framebuffer attachment is incompatible with depth and stencil attachments.");
    }

    DLOG("render", 1) << "Creating OpenGL " << WCC('i') << "Framebuffer" << WCC(0) << "." << std::endl;

    glCreateFramebuffers(1, &rd_handle_);
    glBindFramebuffer(GL_FRAMEBUFFER, rd_handle_);

    DLOGI << "handle:       " << rd_handle_ << std::endl;
    DLOGI << "width*height: " << width_ << "*" << height_ << std::endl;
    DLOGI << "#color bufs:  " << layout.get_count() << std::endl;
    DLOGI << "has cubemap:  " << (has_cubemap() ? "true" : "false") << std::endl;
    DLOGI << "has depth:    " << (has_depth() ? "true" : "false") << std::endl;
    DLOGI << "has stencil:  " << (has_stencil() ? "true" : "false") << std::endl;

    auto* backend = static_cast<OGLBackend*>(gfx::backend.get());
    texture_handles_ = texture_vector.handles;
    cubemap_handle_ = texture_vector.cubemap;

    if(!has_cubemap())
    {
        // * Color buffers
        std::vector<GLenum> draw_buffers;
        int ncolor_attachments = 0;
        for(auto&& elt: layout_)
        {
            // First, create textures for color buffers
            Texture2DDescriptor desc{width_, height_, 0, nullptr, elt.image_format, elt.filter, elt.wrap, TF_NONE};
            const auto& texture = backend->create_texture_inplace(texture_handles_[size_t(ncolor_attachments)], desc);

            // Register color attachment
            GLenum attachment = GLenum(int(GL_COLOR_ATTACHMENT0) + ncolor_attachments++);
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture.get_handle(), 0);
            draw_buffers.push_back(attachment);
        }
        // Specify list of color buffers to draw to
        if(has_depth() && ncolor_attachments == 0) // Depth only texture
            glDrawBuffer(GL_NONE);
        else
            glDrawBuffers(ncolor_attachments, draw_buffers.data());

        // * Handle depth / depth-stencil texture and attachment creation
        if(has_depth() && has_stencil())
        {
            Texture2DDescriptor desc{width_, height_, 0, nullptr, ImageFormat::DEPTH24_STENCIL8, MIN_LINEAR | MAG_NEAREST, TextureWrap::REPEAT, TF_NONE};
            const auto& texture = backend->create_texture_inplace(texture_handles_[size_t(ncolor_attachments)], desc);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture.get_handle(), 0);
        }
        else if(has_depth())
        {
            Texture2DDescriptor desc{width_, height_, 0, nullptr, ImageFormat::DEPTH_COMPONENT24, MIN_LINEAR | MAG_NEAREST, TextureWrap::REPEAT, TF_NONE};
            const auto& texture = backend->create_texture_inplace(texture_handles_[size_t(ncolor_attachments)], desc);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture.get_handle(), 0);
        }

        // No depth texture -> Attach a render buffer to frame buffer as a z-buffer
        // or else, framebuffer is not complete
        if(!has_depth())
        {
            glCreateRenderbuffers(1, &render_buffer_handle_);
            glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_handle_);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width_, height_);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer_handle_);
        }
    }
    else
    {
        W_ASSERT(layout.get_count() == 1, "Framebuffer: only 1 cubemap attachment supported for now.");

        const auto elt = layout[0];

        // First, create cubemap
        CubemapDescriptor desc;
        desc.width = width_;
        desc.height = height_;
        desc.mips = elt.mips;
        desc.image_format = elt.image_format;
        desc.filter = elt.filter;
        desc.wrap = elt.wrap;
        desc.lazy_mipmap = elt.lazy_mipmap;
        const auto& texture = backend->create_cubemap_inplace(cubemap_handle_, desc);


        // Register color attachment
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.get_handle(), 0);

        GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, draw_buffers);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        // glCreateRenderbuffers(1, &render_buffer_handle_);
        // glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_handle_);
        // glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width_, height_);
        // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer_handle_);
    }

    framebuffer_error_report();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

OGLFramebuffer::~OGLFramebuffer()
{
    DLOG("render", 1) << "OpenGL " << WCC('i') << "Framebuffer" << WCC(0) << " [" << rd_handle_ << "] destroyed."
                      << std::endl;
    if(rd_handle_)
        glDeleteFramebuffers(1, &rd_handle_);
    if(render_buffer_handle_)
        glDeleteRenderbuffers(1, &render_buffer_handle_);
}

void OGLFramebuffer::bind(uint32_t mip_level)
{
    if(mip_level == 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, rd_handle_);
        glViewport(0, 0, width_, height_);
    }
    else
    {
        W_ASSERT(has_cubemap(), "[Framebuffer] Target mipmap level only available for cubemap attachements.");
        uint32_t mip_width = std::max(1u, uint32_t(width_ / (1u << mip_level)));
        uint32_t mip_height = std::max(1u, uint32_t(height_ / (1u << mip_level)));
        // glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_handle_);
        // glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mip_width, mip_height);
        auto* backend = static_cast<OGLBackend*>(gfx::backend.get());
        const auto& cm = backend->get_cubemap(cubemap_handle_);
        glBindFramebuffer(GL_FRAMEBUFFER, rd_handle_);
        glViewport(0, 0, mip_width, mip_height);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cm.get_handle(), int(mip_level));
    }
}

void OGLFramebuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

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

void OGLFramebuffer::blit_depth(const OGLFramebuffer& source)
{
    // Push state
    GLint draw_fbo = 0, read_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fbo);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, source.rd_handle_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rd_handle_);
    glBlitFramebuffer(0,                   // src x0
                      0,                   // src y0
                      source.width_,       // src x1
                      source.height_,      // src y1
                      0,                   // dst x0
                      0,                   // dst y0
                      width_,              // dst x1
                      height_,             // dst y1
                      GL_DEPTH_BUFFER_BIT, // mask
                      GL_NEAREST);         // filter

    // Pop state
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fbo);
}

void OGLFramebuffer::framebuffer_error_report()
{
    /*
    GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT is returned if any of the framebuffer attachment points are framebuffer
    incomplete.
    GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT is returned if the framebuffer does not have at least one
    image attached to it.
    GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER is returned if the value of
    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAW_BUFFERi.
    GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER is returned if GL_READ_BUFFER is not GL_NONE and the value of
    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.
    GL_FRAMEBUFFER_UNSUPPORTED is returned if the combination of internal formats of the attached images violates an
    implementation-dependent set of restrictions.
    GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is returned if the value of GL_RENDERBUFFER_SAMPLES is not the same for all
    attached renderbuffers; if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; or, if the
    attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the
    value of GL_TEXTURE_SAMPLES. GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is also returned if the value of
    GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or, if the attached images are a mix of
    renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.
    GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS is returned if any framebuffer attachment
    is layered, and any populated attachment is not layered, or if all populated color attachments are not from textures
    of the same target.
    */

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        switch(status)
        {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            DLOGE("render")
                << "[Framebuffer] Not all framebuffer attachment points are framebuffer attachment complete."
                << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            DLOGE("render") << "[Framebuffer] No images are attached to the framebuffer." << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            DLOGE("render") << "[Framebuffer] Incomplete draw buffer." << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            DLOGE("render") << "[Framebuffer] Incomplete read buffer." << std::endl;
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            DLOGE("render") << "[Framebuffer] The combination of internal formats of the attached images violates an "
                               "implementation-dependent set of restrictions."
                            << std::endl;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            DLOGE("render") << "[Framebuffer] Incomplete multisample." << std::endl;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            DLOGE("render") << "[Framebuffer] Incomplete layer targets." << std::endl;
            break;
        default:
            break;
        }
    }
    else
        DLOG("render", 1) << "Framebuffer [" << rd_handle_ << "] creation " << WCC('g') << "complete" << WCC(0) << "."
                          << std::endl;
}

} // namespace erwin