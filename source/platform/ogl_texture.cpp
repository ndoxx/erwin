#include "platform/ogl_texture.h"
#include "render/render_device.h"
#include "core/core.h"
#include "core/cat_file.h"
#include "debug/logger.h"

#include "glad/glad.h"
#include "stb/stb_image.h"
#include <iostream>
namespace erwin
{


OGLTexture2D::OGLTexture2D(const fs::path filepath):
filepath_(filepath)
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
	width_ = width;
	height_ = height;

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

OGLTexture2D::OGLTexture2D(void* data, uint32_t width, uint32_t height, TextureCompression compression)
{
	DLOGN("texture") << "Loading texture from memory: " << std::endl;
	
	width_ = width;
	height_ = height;

	DLOGI << "WxH:    " << width_ << "x" << height_ << std::endl;
	
	glCreateTextures(GL_TEXTURE_2D, 1, &rd_handle_);

	if(compression == TextureCompression::DXT5)
	{
		DLOGI << "compression: DXT5" << std::endl;
		glTextureStorage2D(rd_handle_, 1, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, width_, height_);
		glCompressedTextureSubImage2D(rd_handle_, 0, 0, 0, width_, height_, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, width*height, data);
	}
	else
	{
		// TMP: Asume 4 channels for now
		glTextureStorage2D(rd_handle_, 1, GL_RGBA8, width_, height_);
		glTextureSubImage2D(rd_handle_, 0, 0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}

	glTextureParameteri(rd_handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(rd_handle_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	DLOGI << "handle: " << rd_handle_ << std::endl;
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

void OGLTexture2D::bind(uint32_t slot)
{
	glBindTextureUnit(slot, rd_handle_);
}

void OGLTexture2D::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}


} // namespace erwin