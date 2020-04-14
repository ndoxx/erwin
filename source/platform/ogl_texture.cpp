#include "platform/ogl_texture.h"
#include "render/render_device.h"
#include "core/core.h"
#include "filesystem/cat_file.h"
#include "debug/logger.h"

#include "glad/glad.h"
#include "glm/glm.hpp"
#include "stb/stb_image.h"
#include <iostream>
namespace erwin
{

struct FormatDescriptor
{
    GLenum internal_format;
    GLenum format;
    GLenum data_type;
    bool is_compressed;
};

static std::map<ImageFormat, FormatDescriptor> FORMAT_DESCRIPTOR =
{
    {ImageFormat::R8,                              {GL_R8,                                  GL_RED,             					GL_UNSIGNED_BYTE,  				   false}},
    {ImageFormat::RGB8,                            {GL_RGB8,                                GL_RGB,             					GL_UNSIGNED_BYTE,  				   false}},
    {ImageFormat::RGBA8,                           {GL_RGBA8,                               GL_RGBA,            					GL_UNSIGNED_BYTE,  				   false}},
    {ImageFormat::RG16F,                           {GL_RG16F,                               GL_RG,              					GL_HALF_FLOAT,     				   false}},
    {ImageFormat::RGB16F,                          {GL_RGB16F,                              GL_RGB,             					GL_HALF_FLOAT,     				   false}},
    {ImageFormat::RGBA16F,                         {GL_RGBA16F,                             GL_RGBA,            					GL_HALF_FLOAT,     				   false}},
    {ImageFormat::RGB32F,                          {GL_RGB32F,                              GL_RGB,             					GL_FLOAT, 	       				   false}},
    {ImageFormat::RGBA32F,                         {GL_RGBA32F,                             GL_RGBA,            					GL_FLOAT, 	       				   false}},
    {ImageFormat::SRGB8,                           {GL_SRGB8,                               GL_RGB,                                 GL_UNSIGNED_BYTE,                  false}},
    {ImageFormat::SRGB_ALPHA,                      {GL_SRGB_ALPHA,                          GL_RGBA,            					GL_UNSIGNED_BYTE,  				   false}},
    {ImageFormat::RG16_SNORM,                      {GL_RG16_SNORM,                          GL_RG,              					GL_SHORT, 		   				   false}},
    {ImageFormat::RGB16_SNORM,                     {GL_RGB16_SNORM,                         GL_RGB,             					GL_SHORT, 		   				   false}},
    {ImageFormat::RGBA16_SNORM,                    {GL_RGBA16_SNORM,                        GL_RGBA,            					GL_SHORT, 		   				   false}},
    {ImageFormat::COMPRESSED_RGB_S3TC_DXT1,        {GL_COMPRESSED_RGB_S3TC_DXT1_EXT,        GL_COMPRESSED_RGB_S3TC_DXT1_EXT,        GL_UNSIGNED_BYTE,  				   true}},
    {ImageFormat::COMPRESSED_RGBA_S3TC_DXT1,       {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,       GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,       GL_UNSIGNED_BYTE,  				   true}},
    {ImageFormat::COMPRESSED_RGBA_S3TC_DXT3,       {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,       GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,       GL_UNSIGNED_BYTE,  				   true}},
    {ImageFormat::COMPRESSED_RGBA_S3TC_DXT5,       {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,       GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,       GL_UNSIGNED_BYTE,  				   true}},
    {ImageFormat::COMPRESSED_SRGB_S3TC_DXT1,       {GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,       GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,       GL_UNSIGNED_BYTE,  				   true}},
    {ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1, {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_UNSIGNED_BYTE,  				   true}},
    {ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT3, {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_UNSIGNED_BYTE,  				   true}},
    {ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5, {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_UNSIGNED_BYTE,  				   true}},
    {ImageFormat::DEPTH_COMPONENT16,               {GL_DEPTH_COMPONENT16,                   GL_DEPTH_COMPONENT, 					GL_UNSIGNED_SHORT, 				   false}},
    {ImageFormat::DEPTH_COMPONENT24,               {GL_DEPTH_COMPONENT24,                   GL_DEPTH_COMPONENT, 					GL_UNSIGNED_INT,   				   false}},
    {ImageFormat::DEPTH_COMPONENT32F,              {GL_DEPTH_COMPONENT32F,                  GL_DEPTH_COMPONENT, 					GL_FLOAT, 						   false}},
    {ImageFormat::DEPTH24_STENCIL8,                {GL_DEPTH24_STENCIL8,                    GL_DEPTH_STENCIL,   					GL_UNSIGNED_INT_24_8, 			   false}},
    {ImageFormat::DEPTH32F_STENCIL8,               {GL_DEPTH32F_STENCIL8,                   GL_DEPTH_STENCIL,   					GL_FLOAT_32_UNSIGNED_INT_24_8_REV, false}},
};

// TODO: group ALL GLenums to string in a SINGLE function outside this class
static std::string format_to_string(GLenum value)
{
	switch(value)
	{
		case GL_RED: return "GL_RED"; break;
		case GL_RGB: return "GL_RGB"; break;
		case GL_RGBA: return "GL_RGBA"; break;
		case GL_RG: return "GL_RG"; break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: return "GL_COMPRESSED_RGB_S3TC_DXT1_EXT"; break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT"; break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT"; break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT"; break;
		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT: return "GL_COMPRESSED_SRGB_S3TC_DXT1_EXT"; break;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT"; break;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT"; break;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT"; break;
		case GL_DEPTH_COMPONENT: return "GL_DEPTH_COMPONENT"; break;
		case GL_DEPTH_STENCIL: return "GL_DEPTH_STENCIL"; break;
		default: return "";
	}
}

static bool handle_filter(uint32_t rd_handle, uint8_t filter)
{
    bool has_mipmap = (filter & TextureFilter::MIN_NEAREST_MIPMAP_NEAREST)
                   || (filter & TextureFilter::MIN_LINEAR_MIPMAP_NEAREST)
                   || (filter & TextureFilter::MIN_NEAREST_MIPMAP_LINEAR)
                   || (filter & TextureFilter::MIN_LINEAR_MIPMAP_LINEAR);

    // Magnification filter
    glTextureParameteri(rd_handle, GL_TEXTURE_MAG_FILTER, bool(filter & MAG_LINEAR) ? GL_LINEAR : GL_NEAREST);

    // Minification filter
    uint16_t minfilter = filter & ~(1 << 0); // Clear mag filter bit

    switch(minfilter)
    {
        case MIN_NEAREST:                glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST); break;
        case MIN_LINEAR:                 glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR); break;
        case MIN_NEAREST_MIPMAP_NEAREST: glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); break;
        case MIN_LINEAR_MIPMAP_NEAREST:  glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); break;
        case MIN_NEAREST_MIPMAP_LINEAR:  glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR); break;
        case MIN_LINEAR_MIPMAP_LINEAR:   glTextureParameteri(rd_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); break;
    }

    return has_mipmap;
}

static void handle_address_UV_2D(uint32_t rd_handle, TextureWrap wrap)
{
    GLint param;
    switch(wrap)
    {
        case TextureWrap::REPEAT:          param = GL_REPEAT; break;
        case TextureWrap::MIRRORED_REPEAT: param = GL_MIRRORED_REPEAT; break;
        case TextureWrap::CLAMP_TO_EDGE:   param = GL_CLAMP_TO_EDGE; break;
    }
    glTextureParameteri(rd_handle, GL_TEXTURE_WRAP_S, param);
    glTextureParameteri(rd_handle, GL_TEXTURE_WRAP_T, param);
}

static void handle_address_UV_3D(uint32_t rd_handle, TextureWrap wrap)
{
    GLint param;
    switch(wrap)
    {
        case TextureWrap::REPEAT:          param = GL_REPEAT; break;
        case TextureWrap::MIRRORED_REPEAT: param = GL_MIRRORED_REPEAT; break;
        case TextureWrap::CLAMP_TO_EDGE:   param = GL_CLAMP_TO_EDGE; break;
    }
    glTextureParameteri(rd_handle, GL_TEXTURE_WRAP_S, param);
    glTextureParameteri(rd_handle, GL_TEXTURE_WRAP_T, param);
    glTextureParameteri(rd_handle, GL_TEXTURE_WRAP_R, param);
}

static void generate_mipmaps(uint32_t rd_handle, uint32_t base_level, uint32_t max_level)
{
    W_ASSERT(max_level>=base_level, "Max mipmap level must be greater than base mipmap level.");

    DLOG("texture",1) << "Generating mipmaps: " << base_level << " - " << max_level << std::endl;

    glTextureParameteri(rd_handle, GL_TEXTURE_BASE_LEVEL, base_level);
    glTextureParameteri(rd_handle, GL_TEXTURE_MAX_LEVEL, max_level);
    glGenerateTextureMipmap(rd_handle);

    GLfloat max_anisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
    glTextureParameterf(rd_handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, glm::clamp(max_anisotropy, 0.0f, 8.0f));
}


OGLTexture2D::OGLTexture2D(const fs::path filepath)
{
	DLOGN("texture") << "Loading texture from file: " << std::endl;
	DLOGI << "path:   " << WCC('p') << fs::relative(filepath, filesystem::get_asset_dir()) << WCC(0) << std::endl;
	
	if(!fs::exists(filepath))
	{
		DLOGW("texture") << "File does not exist!" << std::endl;
		DLOGI << "Loading " << WCC('d') << "default" << WCC(0) << std::endl;
		W_ASSERT(false, "Default texture loading not implemented!");
	}

	int width, height, channels;
	// Need to flip vertically so that data is in the expected order for OpenGL
	stbi_set_flip_vertically_on_load(1);

	// 5th parameter can be used to force a format like RGBA when input file is just RGB
	stbi_uc* data = stbi_load(filepath.string().c_str(), &width, &height, &channels, 0);
	W_ASSERT(data, "Failed to load image!");
	width_ = uint32_t(width);
	height_ = uint32_t(height);

	DLOGI << "WxH:    " << width_ << "x" << height_ << std::endl;

	// Setup internal format using number of channels
	GLenum internalFormat = 0, dataFormat = 0;
	if(channels == 4)
	{
		internalFormat = GL_RGBA8;
		dataFormat = GL_RGBA;
	}
	else if(channels == 3)
	{
		internalFormat = GL_RGB8;
		dataFormat = GL_RGB;
	}
	W_ASSERT(internalFormat & dataFormat, "Format not supported!");

	// Upload to OpenGL
	glCreateTextures(GL_TEXTURE_2D, 1, &rd_handle_);
	glTextureStorage2D(rd_handle_, 1, internalFormat, width_, height_);
	glTextureParameteri(rd_handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(rd_handle_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureSubImage2D(rd_handle_, 0, 0, 0, width_, height_, dataFormat, GL_UNSIGNED_BYTE, data);

	DLOGI << "handle: " << rd_handle_ << std::endl;

	// Cleanup
	stbi_image_free(data);
}

OGLTexture2D::OGLTexture2D(const Texture2DDescriptor& descriptor):
width_(descriptor.width),
height_(descriptor.height)
{
	DLOGN("texture") << "Creating texture from descriptor: " << std::endl;

	glCreateTextures(GL_TEXTURE_2D, 1, &rd_handle_);
	DLOGI << "handle: " << rd_handle_ << std::endl;
	DLOGI << "width:  " << width_ << std::endl;
	DLOGI << "height: " << height_ << std::endl;

	const FormatDescriptor& fd = FORMAT_DESCRIPTOR.at(descriptor.image_format);
	glTextureStorage2D(rd_handle_, 1, fd.internal_format, width_, height_);
	DLOGI << "format: " << format_to_string(fd.format) << std::endl;

	if(descriptor.data)
	{
		if(fd.is_compressed)
			glCompressedTextureSubImage2D(rd_handle_, 0, 0, 0, width_, height_, fd.format, width_*height_, descriptor.data);
		else
			glTextureSubImage2D(rd_handle_, 0, 0, 0, width_, height_, fd.format, fd.data_type, descriptor.data);
	}

	bool has_mipmap = handle_filter(rd_handle_, descriptor.filter);
	DLOGI << "mipmap: " << (has_mipmap ? "true" : "false") << std::endl;
	handle_address_UV_2D(rd_handle_, descriptor.wrap);

    // Handle mipmap if specified
    if(has_mipmap && !descriptor.lazy_mipmap)
        generate_mipmaps(rd_handle_, 0, 3);
    else
    {
        glTextureParameteri(rd_handle_, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(rd_handle_, GL_TEXTURE_MAX_LEVEL, 0);
    }
}

OGLTexture2D::~OGLTexture2D()
{
	glDeleteTextures(1, &rd_handle_);
	DLOG("texture",1) << "Destroyed texture [" << rd_handle_ << "]" << std::endl;
}

uint32_t OGLTexture2D::get_width() const
{
	return width_;
}

uint32_t OGLTexture2D::get_height() const
{
	return height_;
}

void OGLTexture2D::bind(uint32_t slot) const
{
	glBindTextureUnit(slot, rd_handle_);
}

void OGLTexture2D::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void* OGLTexture2D::get_native_handle()
{
    // Cast to void* directly for compatibility with ImGUI
    return reinterpret_cast<void*>(uint64_t(rd_handle_));
}





OGLCubemap::OGLCubemap(const CubemapDescriptor& descriptor):
width_(descriptor.width),
height_(descriptor.height)
{
    DLOGN("texture") << "Creating cubemap from descriptor: " << std::endl;

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &rd_handle_);
    DLOGI << "handle: " << rd_handle_ << std::endl;
    DLOGI << "width:  " << width_ << std::endl;
    DLOGI << "height: " << height_ << std::endl;

    const FormatDescriptor& fd = FORMAT_DESCRIPTOR.at(descriptor.image_format);
    glTextureStorage2D(rd_handle_, 1, fd.internal_format, width_, height_);

    bool has_data = true;
    for(size_t face=0; face<6; ++face)
        has_data &= (descriptor.face_data[face] != nullptr);

    if(has_data)
    {
        for(size_t face=0; face<6; ++face)
        {
            if(fd.is_compressed)
                glCompressedTextureSubImage3D(rd_handle_, 0, 0, 0, int(face), width_, height_, 1, fd.format, width_*height_, descriptor.face_data[face]);
            else
                glTextureSubImage3D(rd_handle_, 0, 0, 0, int(face), width_, height_, 1, fd.format, fd.data_type, descriptor.face_data[face]);
        }
    }

    bool has_mipmap = handle_filter(rd_handle_, descriptor.filter);
    DLOGI << "mipmap: " << (has_mipmap ? "true" : "false") << std::endl;
    handle_address_UV_3D(rd_handle_, descriptor.wrap);

    // Handle mipmap if specified
    if(has_mipmap && !descriptor.lazy_mipmap)
        generate_mipmaps(rd_handle_, 0, 3);
    else
    {
        glTextureParameteri(rd_handle_, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(rd_handle_, GL_TEXTURE_MAX_LEVEL, 0);
    }
}

OGLCubemap::~OGLCubemap()
{
    glDeleteTextures(1, &rd_handle_);
    DLOG("texture",1) << "Destroyed cubemap [" << rd_handle_ << "]" << std::endl;
}

uint32_t OGLCubemap::get_width() const
{
    return width_;
}

uint32_t OGLCubemap::get_height() const
{
    return height_;
}

void OGLCubemap::bind(uint32_t slot) const
{
    glBindTextureUnit(slot, rd_handle_);
}

void OGLCubemap::unbind() const
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void* OGLCubemap::get_native_handle()
{
    // Cast to void* directly for compatibility with ImGUI
    return reinterpret_cast<void*>(uint64_t(rd_handle_));
}




} // namespace erwin