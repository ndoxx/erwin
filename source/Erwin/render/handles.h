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

static constexpr std::size_t k_render_handle_alloc_size =
    HandlePoolT<k_max_handles<IndexBufferHandle>>::k_size_bytes +
    HandlePoolT<k_max_handles<VertexBufferLayoutHandle>>::k_size_bytes +
    HandlePoolT<k_max_handles<VertexBufferHandle>>::k_size_bytes + 
    HandlePoolT<k_max_handles<VertexArrayHandle>>::k_size_bytes +
    HandlePoolT<k_max_handles<UniformBufferHandle>>::k_size_bytes +
    HandlePoolT<k_max_handles<ShaderStorageBufferHandle>>::k_size_bytes + 
    HandlePoolT<k_max_handles<TextureHandle>>::k_size_bytes +
    HandlePoolT<k_max_handles<CubemapHandle>>::k_size_bytes + 
    HandlePoolT<k_max_handles<ShaderHandle>>::k_size_bytes +
    HandlePoolT<k_max_handles<FramebufferHandle>>::k_size_bytes +
    1_kB;

} // namespace erwin