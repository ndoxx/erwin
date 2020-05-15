#pragma once

#include <cstdint>
#include "render/handles.h"

namespace erwin
{

enum class RenderCommand: uint16_t
{
	CreateIndexBuffer,
	CreateVertexBuffer,
	CreateVertexArray,
	CreateVertexArrayMultipleVBO,
	CreateUniformBuffer,
	CreateShaderStorageBuffer,
	CreateShader,
	CreateTexture2D,
	CreateCubemap,
	CreateFramebuffer,

	UpdateIndexBuffer,
	UpdateVertexBuffer,
	UpdateUniformBuffer,
	UpdateShaderStorageBuffer,
	ShaderAttachUniformBuffer,
	ShaderAttachStorageBuffer,
	UpdateFramebuffer,
	ClearFramebuffers,
	SetHostWindowSize,

	Post,

	GetPixelData,
	GenerateCubemapMipmaps,
	FramebufferScreenshot,

	DestroyIndexBuffer,
	DestroyVertexBufferLayout,
	DestroyVertexBuffer,
	DestroyVertexArray,
	DestroyUniformBuffer,
	DestroyShaderStorageBuffer,
	DestroyShader,
	DestroyTexture2D,
	DestroyCubemap,
	DestroyFramebuffer,

	Count
};

enum class DrawCommand: uint16_t
{
	Draw,
	Clear,
	BlitDepth,
	UpdateShaderStorageBuffer,
	UpdateUniformBuffer,

	Count
};



// All the state needed by the renderer to perform a platform draw call
struct DrawCall
{
	enum DrawCallType: uint8_t
	{
		Indexed,
		Array,
		IndexedInstanced,
		ArrayInstanced,

		Count
	};

	#pragma pack(push,1)
	struct Data
	{
		uint64_t state_flags;
		uint32_t count;
		uint32_t offset;

		ShaderHandle shader;
		VertexArrayHandle VAO;
	} data;
	#pragma pack(pop)
	uint32_t instance_count;
	uint32_t dependencies[k_max_draw_call_dependencies];
	TextureHandle textures[k_max_texture_slots];
	CubemapHandle cubemaps[k_max_cubemap_slots];
	DrawCallType type;
	uint8_t dependency_count;
	uint8_t texture_count;
	uint8_t cubemap_count;

	DrawCall(DrawCallType dc_type, uint64_t state, ShaderHandle shader, VertexArrayHandle VAO, uint32_t count=0, uint32_t offset=0)
	{
		W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");
		W_ASSERT(VAO.is_valid(), "Invalid VertexArrayHandle!");

		instance_count   = 0;
		dependency_count = 0;
		texture_count    = 0;
		cubemap_count    = 0;
		type             = dc_type;
		data.state_flags = state;
		data.shader      = shader;
		data.VAO         = VAO;
		data.count       = count;
		data.offset      = offset;
	}

	inline void add_dependency(uint32_t token)
	{
		W_ASSERT(dependency_count<k_max_draw_call_dependencies-1, "Exceeding draw call max dependency count.");
		dependencies[dependency_count++] = token;
	}

	inline void set_instance_count(uint32_t value)
	{
		instance_count = value;
	}

	// Set a texture at next slot
	inline void add_texture(TextureHandle tex)
	{
		W_ASSERT_FMT(texture_count<k_max_texture_slots-1, "Texture slot out of bounds: %u", texture_count);
		textures[texture_count++] = tex;
	}

	// Set a texture at a given slot
	inline void set_texture(TextureHandle tex, uint32_t slot=0)
	{
		W_ASSERT_FMT(slot<k_max_texture_slots, "Texture slot out of bounds: %u", slot);
		textures[slot] = tex;
		++texture_count;
	}

	// Set a cubemap at next slot
	inline void add_cubemap(CubemapHandle cm)
	{
		W_ASSERT_FMT(cm.is_valid(), "Invalid CubemapHandle of index: %hu", cm.index);
		W_ASSERT_FMT(cubemap_count<k_max_cubemap_slots-1, "Cubemap slot out of bounds: %u", cubemap_count);
		cubemaps[cubemap_count++] = cm;
	}

	// Set a cubemap at a given slot
	inline void set_cubemap(CubemapHandle cm, uint32_t slot=0)
	{
		W_ASSERT_FMT(cm.is_valid(), "Invalid CubemapHandle of index: %hu", cm.index);
		W_ASSERT_FMT(slot<k_max_cubemap_slots, "Texture slot out of bounds: %u", slot);
		cubemaps[slot] = cm;
		++cubemap_count;
	}
};

} // namespace erwin