#pragma once

namespace erwin
{
namespace WIP
{

// Handle structures to manipulate graphics objects
enum HandleType: uint16_t
{
	IndexBufferHandleT,
	VertexBufferLayoutHandleT,
	VertexBufferHandleT,
	VertexArrayHandleT,
	UniformBufferHandleT,
	ShaderStorageBufferHandleT,
	TextureHandleT,
	ShaderHandleT,
	FramebufferHandleT,

	Count
};

#define W_HANDLE(name)   							               				\
	struct name 		 							               				\
	{ 					 							               				\
		inline bool operator ==(const name& o) const { return index==o.index; } \
		inline bool operator !=(const name& o) const { return index!=o.index; } \
		static constexpr HandleType type = HandleType::name##T;    				\
		uint32_t index = 0xffff;	 							  			   	\
	};					 							  			   				\
	extern bool is_valid(name); 							  			   		\

W_HANDLE(IndexBufferHandle);
W_HANDLE(VertexBufferLayoutHandle);
W_HANDLE(VertexBufferHandle);
W_HANDLE(VertexArrayHandle);
W_HANDLE(UniformBufferHandle);
W_HANDLE(ShaderStorageBufferHandle);
W_HANDLE(TextureHandle);
W_HANDLE(ShaderHandle);
W_HANDLE(FramebufferHandle);

#undef W_HANDLE

constexpr std::size_t k_max_render_commands = 10000;
constexpr std::size_t k_max_handles[HandleType::Count] =
{
	128, // index buffers
	128, // vertex buffer layouts
	128, // vertex buffers
	128, // vertex arrays
	128, // uniform buffers
	128, // shader storage buffers
	128, // textures
	128, // shaders
	32,  // framebuffers
};

} // namespace WIP
} // namespace erwin