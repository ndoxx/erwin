#pragma once

#include "core/handle.h"
#include "render/renderer_config.h"

namespace erwin
{

HANDLE_DECLARATION(IndexBufferHandle, 512);
HANDLE_DECLARATION(VertexBufferLayoutHandle, 64);
HANDLE_DECLARATION(VertexBufferHandle, 512);
HANDLE_DECLARATION(VertexArrayHandle, 512);
HANDLE_DECLARATION(UniformBufferHandle, 256);
HANDLE_DECLARATION(ShaderStorageBufferHandle, 64);
HANDLE_DECLARATION(TextureHandle, 256);
HANDLE_DECLARATION(CubemapHandle, 256);
HANDLE_DECLARATION(ShaderHandle, 256);
HANDLE_DECLARATION(FramebufferHandle, 256);

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