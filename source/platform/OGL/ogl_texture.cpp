#include "platform/OGL/ogl_texture.h"
#include "core/core.h"
#include <kibble/logger/logger.h>

#include "glad/glad.h"
#include "glm/glm.hpp"
#include <iostream>
#include <map>



namespace erwin
{

struct FormatDescriptor
{
    GLenum internal_format;
    GLenum format;
    GLenum data_type;
    bool is_compressed;
};

static const std::map<ImageFormat, FormatDescriptor> s_format_descriptor = {
    {ImageFormat::R8, {GL_R8, GL_RED, GL_UNSIGNED_BYTE, false}},
    {ImageFormat::RGB8, {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, false}},
    {ImageFormat::RGBA8, {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, false}},
    {ImageFormat::RG16F, {GL_RG16F, GL_RG, GL_FLOAT, false}},
    {ImageFormat::RGB16F, {GL_RGB16F, GL_RGB, GL_FLOAT, false}},
    {ImageFormat::RGBA16F, {GL_RGBA16F, GL_RGBA, GL_FLOAT, false}},
    {ImageFormat::RGB32F, {GL_RGB32F, GL_RGB, GL_FLOAT, false}},
    {ImageFormat::RGBA32F, {GL_RGBA32F, GL_RGBA, GL_FLOAT, false}},
    {ImageFormat::SRGB8, {GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE, false}},
    {ImageFormat::SRGB_ALPHA, {GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, false}},
    {ImageFormat::RG16_SNORM, {GL_RG16_SNORM, GL_RG, GL_SHORT, false}},
    {ImageFormat::RGB16_SNORM, {GL_RGB16_SNORM, GL_RGB, GL_SHORT, false}},
    {ImageFormat::RGBA16_SNORM, {GL_RGBA16_SNORM, GL_RGBA, GL_SHORT, false}},
    {ImageFormat::COMPRESSED_RGB_S3TC_DXT1,
     {GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE, true}},
    {ImageFormat::COMPRESSED_RGBA_S3TC_DXT1,
     {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE, true}},
    {ImageFormat::COMPRESSED_RGBA_S3TC_DXT3,
     {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_UNSIGNED_BYTE, true}},
    {ImageFormat::COMPRESSED_RGBA_S3TC_DXT5,
     {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_UNSIGNED_BYTE, true}},
    {ImageFormat::COMPRESSED_SRGB_S3TC_DXT1,
     {GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE, true}},
    {ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1,
     {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE, true}},
    {ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT3,
     {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_UNSIGNED_BYTE, true}},
    {ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5,
     {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_UNSIGNED_BYTE, true}},
    {ImageFormat::DEPTH_COMPONENT16, {GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, false}},
    {ImageFormat::DEPTH_COMPONENT24, {GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, false}},
    {ImageFormat::DEPTH_COMPONENT32F, {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, false}},
    {ImageFormat::DEPTH24_STENCIL8, {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, false}},
    {ImageFormat::DEPTH32F_STENCIL8,
     {GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, false}},
};

// TODO: group ALL GLenums to string in a SINGLE function outside this class
[[maybe_unused]] static std::string format_to_string(GLenum value)
{
    switch(value)
    {
    case GL_RED:
        return "GL_RED";
        break;
    case GL_RGB:
        return "GL_RGB";
        break;
    case GL_RGBA:
        return "GL_RGBA";
        break;
    case GL_RG:
        return "GL_RG";
        break;
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        return "GL_COMPRESSED_RGB_S3TC_DXT1_EXT";
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT";
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        return "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
        break;
    case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        return "GL_COMPRESSED_SRGB_S3TC_DXT1_EXT";
        break;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT";
        break;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT";
        break;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT";
        break;
    case GL_DEPTH_COMPONENT:
        return "GL_DEPTH_COMPONENT";
        break;
    case GL_DEPTH_STENCIL:
        return "GL_DEPTH_STENCIL";
        break;
    default:
        return "";
    }
}

static bool handle_filter(uint32_t rd_handle, uint8_t filter)
{
    bool has_mipmap =
        (filter & TextureFilter::MIN_NEAREST_MIPMAP_NEAREST) || (filter & TextureFilter::MIN_LINEAR_MIPMAP_NEAREST) ||
        (filter & TextureFilter::MIN_NEAREST_MIPMAP_LINEAR) || (filter & TextureFilter::MIN_LINEAR_MIPMAP_LINEAR);

    // Magnification filter
    glTextureParameteri(rd_handle, GL_TEXTURE_MAG_FILTER, bool(filter & MAG_LINEAR) ? GL_LINEAR : GL_NEAREST);

    // Minification filter
    uint16_t minfilter = filter & ~(1 << 0); // Clear mag filter bit

    switch(minfilter)
    {
    case MIN_NEAREST:
        glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    case MIN_LINEAR:
        glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    case MIN_NEAREST_MIPMAP_NEAREST:
        glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        break;
    case MIN_LINEAR_MIPMAP_NEAREST:
        glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        break;
    case MIN_NEAREST_MIPMAP_LINEAR:
        glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        break;
    case MIN_LINEAR_MIPMAP_LINEAR:
        glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
    }

    return has_mipmap;
}

static void handle_address_UV_2D(uint32_t rd_handle, TextureWrap wrap)
{
    GLint param;
    switch(wrap)
    {
    case TextureWrap::REPEAT:
        param = GL_REPEAT;
        break;
    case TextureWrap::MIRRORED_REPEAT:
        param = GL_MIRRORED_REPEAT;
        break;
    case TextureWrap::CLAMP_TO_EDGE:
        param = GL_CLAMP_TO_EDGE;
        break;
    }
    glTextureParameteri(rd_handle, GL_TEXTURE_WRAP_S, param);
    glTextureParameteri(rd_handle, GL_TEXTURE_WRAP_T, param);
}

static void handle_address_UV_3D(uint32_t rd_handle, TextureWrap wrap)
{
    GLint param;
    switch(wrap)
    {
    case TextureWrap::REPEAT:
        param = GL_REPEAT;
        break;
    case TextureWrap::MIRRORED_REPEAT:
        param = GL_MIRRORED_REPEAT;
        break;
    case TextureWrap::CLAMP_TO_EDGE:
        param = GL_CLAMP_TO_EDGE;
        break;
    }
    glTextureParameteri(rd_handle, GL_TEXTURE_WRAP_S, param);
    glTextureParameteri(rd_handle, GL_TEXTURE_WRAP_T, param);
    glTextureParameteri(rd_handle, GL_TEXTURE_WRAP_R, param);
}

static void do_generate_mipmaps(uint32_t rd_handle, uint32_t base_level, uint32_t max_level)
{
    K_ASSERT(max_level >= base_level, "Max mipmap level must be greater than base mipmap level.");

    KLOG("texture", 1) << "Generating mipmaps: " << base_level << " - " << max_level << std::endl;

    glTextureParameteri(rd_handle, GL_TEXTURE_BASE_LEVEL, base_level);
    glTextureParameteri(rd_handle, GL_TEXTURE_MAX_LEVEL, max_level);
    glGenerateTextureMipmap(rd_handle);

    GLfloat max_anisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
    glTextureParameterf(rd_handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, glm::clamp(max_anisotropy, 0.0f, 8.0f));
}

OGLTexture2D::OGLTexture2D(const Texture2DDescriptor& descriptor) { init(descriptor); }

OGLTexture2D::~OGLTexture2D() { release(); }

void OGLTexture2D::init(const Texture2DDescriptor& descriptor)
{
    if(!initialized_)
    {
        width_ = descriptor.width;
        height_ = descriptor.height;
        mips_ = descriptor.mips;
        format_ = descriptor.image_format;
        filter_ = descriptor.filter;
        wrap_ = descriptor.wrap;

        KLOGN("texture") << "Creating texture from descriptor: " << std::endl;

        glCreateTextures(GL_TEXTURE_2D, 1, &rd_handle_);
        KLOGI << "handle: " << rd_handle_ << std::endl;
        KLOGI << "width:  " << width_ << std::endl;
        KLOGI << "height: " << height_ << std::endl;

        const FormatDescriptor& fd = s_format_descriptor.at(descriptor.image_format);
        glTextureStorage2D(rd_handle_, mips_ + 1, fd.internal_format, width_, height_);
        KLOGI << "format: " << format_to_string(fd.format) << std::endl;

        if(descriptor.data)
        {
            if(fd.is_compressed)
                glCompressedTextureSubImage2D(rd_handle_, 0, 0, 0, width_, height_, fd.format, width_ * height_,
                                              descriptor.data);
            else
                glTextureSubImage2D(rd_handle_, 0, 0, 0, width_, height_, fd.format, fd.data_type, descriptor.data);
        }

        bool has_mipmap = handle_filter(rd_handle_, descriptor.filter);
        if(has_mipmap)
        {
            KLOGI << "mipmap: " << kb::KS_POS_ << "true" << std::endl;
            KLOGI << "levels: " << mips_ << std::endl;
        }
        else
        {
            KLOGI << "mipmap: " << kb::KS_NEG_ << "false" << std::endl;
        }
        handle_address_UV_2D(rd_handle_, descriptor.wrap);

        // Handle mipmap if specified
        if(has_mipmap && !descriptor.lazy_mipmap() && mips_ > 0)
            do_generate_mipmaps(rd_handle_, 0, mips_);
        else
        {
            glTextureParameteri(rd_handle_, GL_TEXTURE_BASE_LEVEL, 0);
            glTextureParameteri(rd_handle_, GL_TEXTURE_MAX_LEVEL, 0);
        }

        initialized_ = true;
    }
}

void OGLTexture2D::release()
{
    if(initialized_)
    {
        glDeleteTextures(1, &rd_handle_);
        KLOG("texture", 1) << "Destroyed texture [" << rd_handle_ << "]" << std::endl;
        initialized_ = false;
    }
}

void OGLTexture2D::generate_mipmaps() const { do_generate_mipmaps(rd_handle_, 0, mips_); }

std::pair<uint8_t*, size_t> OGLTexture2D::read_pixels() const
{
    size_t bufsize = 4 * width_ * height_;
    uint8_t* buffer = new uint8_t[bufsize];
    glGetTextureImage(rd_handle_, 0, GL_RGBA, GL_UNSIGNED_BYTE, GLsizei(bufsize), buffer);
    return {buffer, bufsize};
}

void OGLTexture2D::bind(uint32_t slot) const { glBindTextureUnit(slot, rd_handle_); }

void OGLTexture2D::unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

OGLCubemap::OGLCubemap(const CubemapDescriptor& descriptor) { init(descriptor); }

OGLCubemap::~OGLCubemap() { release(); }

void OGLCubemap::init(const CubemapDescriptor& descriptor)
{
    if(!initialized_)
    {
        width_ = descriptor.width;
        height_ = descriptor.height;
        mips_ = descriptor.mips;
        format_ = descriptor.image_format;
        filter_ = descriptor.filter;
        wrap_ = descriptor.wrap;

        KLOGN("texture") << "Creating cubemap from descriptor: " << std::endl;

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &rd_handle_);
        KLOGI << "handle: " << rd_handle_ << std::endl;
        KLOGI << "width:  " << width_ << std::endl;
        KLOGI << "height: " << height_ << std::endl;

        const FormatDescriptor& fd = s_format_descriptor.at(descriptor.image_format);
        glTextureStorage2D(rd_handle_, mips_ + 1, fd.internal_format, width_, height_);
        KLOGI << "format: " << format_to_string(fd.format) << std::endl;

        bool has_data = true;
        for(size_t face = 0; face < 6; ++face)
            has_data &= (descriptor.face_data[face] != nullptr);

        if(has_data)
        {
            for(size_t face = 0; face < 6; ++face)
            {
                if(fd.is_compressed)
                    glCompressedTextureSubImage3D(rd_handle_, 0, 0, 0, int(face), width_, height_, 1, fd.format,
                                                  width_* height_, descriptor.face_data[face]);
                else
                    glTextureSubImage3D(rd_handle_, 0, 0, 0, int(face), width_, height_, 1, fd.format, fd.data_type,
                                        descriptor.face_data[face]);
            }
        }

        bool has_mipmap = handle_filter(rd_handle_, descriptor.filter);
        if(has_mipmap)
        {
            KLOGI << "mipmap: " << kb::KS_POS_ << "true" << std::endl;
            KLOGI << "levels: " << mips_ << std::endl;
        }
        else
        {
            KLOGI << "mipmap: " << kb::KS_NEG_ << "false" << std::endl;
        }
        handle_address_UV_3D(rd_handle_, descriptor.wrap);

        // Handle mipmap if specified
        if(has_mipmap && !descriptor.lazy_mipmap && mips_ > 0)
            do_generate_mipmaps(rd_handle_, 0, mips_);
        else
        {
            glTextureParameteri(rd_handle_, GL_TEXTURE_BASE_LEVEL, 0);
            glTextureParameteri(rd_handle_, GL_TEXTURE_MAX_LEVEL, 0);
        }

        initialized_ = true;
    }
}

void OGLCubemap::release()
{
    if(initialized_)
    {
        glDeleteTextures(1, &rd_handle_);
        KLOG("texture", 1) << "Destroyed cubemap [" << rd_handle_ << "]" << std::endl;
        initialized_ = false;
    }
}

void OGLCubemap::generate_mipmaps() const { do_generate_mipmaps(rd_handle_, 0, mips_); }

void OGLCubemap::bind(uint32_t slot) const { glBindTextureUnit(slot, rd_handle_); }

void OGLCubemap::unbind() const { glBindTexture(GL_TEXTURE_CUBE_MAP, 0); }

} // namespace erwin