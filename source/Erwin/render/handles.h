#pragma once

#include "core/handle.h"

namespace erwin
{

HANDLE_DECLARATION( IndexBufferHandle );
HANDLE_DECLARATION( VertexBufferLayoutHandle );
HANDLE_DECLARATION( VertexBufferHandle );
HANDLE_DECLARATION( VertexArrayHandle );
HANDLE_DECLARATION( UniformBufferHandle );
HANDLE_DECLARATION( ShaderStorageBufferHandle );
HANDLE_DECLARATION( TextureHandle );
HANDLE_DECLARATION( ShaderHandle );
HANDLE_DECLARATION( FramebufferHandle );

static constexpr std::size_t k_max_render_handles = 128;

} // namespace erwin