#pragma once

#include <cstdint>
#include <cstring>

#include "core/core.h"
#include "render/render_state.h"

namespace erwin
{
namespace WIP
{

// Handle structures to manipulate graphics objects
constexpr uint16_t k_invalid_handle = 0xffff;

#define W_HANDLE(name)                                                     \
	struct name 														   \
	{ 																	   \
		inline bool is_valid() const { return index != k_invalid_handle; } \
		inline void invalidate()     { index = k_invalid_handle; }         \
		uint16_t index;													   \
	};																	   \

W_HANDLE(IndexBufferHandle);
W_HANDLE(VertexBufferHandle);
W_HANDLE(VertexArrayHandle);
W_HANDLE(UniformBufferHandle);
W_HANDLE(ShaderStorageBufferHandle);
W_HANDLE(TextureHandle);
W_HANDLE(ShaderHandle);

constexpr std::size_t k_max_render_command_data_size = 32;

// Pushed to a command queue
struct RenderCommand
{
	enum: uint16_t
	{
		CreateIndexBuffer,
		CreateVertexBuffer,
		CreateVertexArray,
		CreateUniformBuffer,
		CreateShaderStorageBuffer,

		Count
	};

	inline void reset() { flags = 0; head = 0; auxiliary = nullptr; backend_dispatch_func = nullptr; }
	inline void write(void const* source, std::size_t size)
	{
		W_ASSERT(size + head < k_max_render_command_data_size, "[RenderCommand] Data buffer overwrite!");
		memcpy(data + head, source, size);
		head += (uint16_t)size;
	}
	inline void read(void* destination, std::size_t size)
	{
		W_ASSERT(int(head) - size >= 0, "[RenderCommand] Data buffer empty!");
		memcpy(destination, data + head - size, size);
		head -= (uint16_t)size;
	}

	void create_index_buffer(IndexBufferHandle* handle, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode = DrawMode::Static);

	uint64_t flags;
	uint16_t type;
	uint16_t head;
	uint8_t  data[k_max_render_command_data_size];

	void* auxiliary;
	void (*backend_dispatch_func)(RenderCommand*);
};

class CommandQueue
{
public:

};

} // namespace WIP
} // namespace erwin