#pragma once

#include "core/handle.h"
#include "render/renderer_config.h"

namespace erwin
{

HANDLE_DECLARATION(IndexBufferHandle, k_max_index_buffers);
HANDLE_DECLARATION(VertexBufferLayoutHandle, k_max_vertex_buffer_layouts);
HANDLE_DECLARATION(VertexBufferHandle, k_max_vertex_buffers);
HANDLE_DECLARATION(VertexArrayHandle, k_max_vertex_arrays);
HANDLE_DECLARATION(UniformBufferHandle, k_max_uniform_buffers);
HANDLE_DECLARATION(ShaderStorageBufferHandle, k_max_shader_storage_buffers);
HANDLE_DECLARATION(TextureHandle, k_max_textures);
HANDLE_DECLARATION(CubemapHandle, k_max_cubemaps);
HANDLE_DECLARATION(ShaderHandle, k_max_shaders);
HANDLE_DECLARATION(FramebufferHandle, k_max_framebuffers);

[[maybe_unused]] static constexpr std::size_t k_render_handle_alloc_size =
    sizeof(HandlePoolT<k_max_handles<IndexBufferHandle>>) +
    sizeof(HandlePoolT<k_max_handles<VertexBufferLayoutHandle>>) +
    sizeof(HandlePoolT<k_max_handles<VertexBufferHandle>>) + 
    sizeof(HandlePoolT<k_max_handles<VertexArrayHandle>>) +
    sizeof(HandlePoolT<k_max_handles<UniformBufferHandle>>) +
    sizeof(HandlePoolT<k_max_handles<ShaderStorageBufferHandle>>) + 
    sizeof(HandlePoolT<k_max_handles<TextureHandle>>) +
    sizeof(HandlePoolT<k_max_handles<CubemapHandle>>) + 
    sizeof(HandlePoolT<k_max_handles<ShaderHandle>>) +
    sizeof(HandlePoolT<k_max_handles<FramebufferHandle>>) +
    1_kB;

} // namespace erwin