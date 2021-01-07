#pragma once

#include <cstdint>

#ifdef W_PROFILE_RENDER
#include "debug/instrumentor.h"
#define W_PROFILE_RENDER_SCOPE(name) InstrumentationTimer timer##__LINE__(name);
#define W_PROFILE_RENDER_FUNCTION() W_PROFILE_RENDER_SCOPE(__PRETTY_FUNCTION__)
#else
#define W_PROFILE_RENDER_SCOPE(name)
#define W_PROFILE_RENDER_FUNCTION()
#endif

namespace erwin
{

#define W_RC_PROFILE_DRAW_CALLS true

// Maximum amount of texture slots per draw call
[[maybe_unused]] static constexpr uint32_t k_max_texture_slots = 32;
// Maximum amount of cubemap slots per draw call
[[maybe_unused]] static constexpr uint32_t k_max_cubemap_slots = 8;
// Maximum amount of render commands per frame, per command buffer
[[maybe_unused]] static constexpr uint32_t k_max_render_commands = 2048;
// Maximum amount of draw calls per frame
[[maybe_unused]] static constexpr uint32_t k_max_draw_calls = 8192;
// Maximum amount of dependencies per draw call
[[maybe_unused]] static constexpr uint32_t k_max_draw_call_dependencies = 8;

// Maximum amount of managed objects
[[maybe_unused]] static constexpr uint32_t k_max_index_buffers = 512;
[[maybe_unused]] static constexpr uint32_t k_max_vertex_buffer_layouts = 64;
[[maybe_unused]] static constexpr uint32_t k_max_vertex_buffers = 512;
[[maybe_unused]] static constexpr uint32_t k_max_vertex_arrays = 512;
[[maybe_unused]] static constexpr uint32_t k_max_uniform_buffers = 256;
[[maybe_unused]] static constexpr uint32_t k_max_shader_storage_buffers = 64;
[[maybe_unused]] static constexpr uint32_t k_max_textures = 256;
[[maybe_unused]] static constexpr uint32_t k_max_cubemaps = 256;
[[maybe_unused]] static constexpr uint32_t k_max_shaders = 256;
[[maybe_unused]] static constexpr uint32_t k_max_framebuffers = 256;

// DEBUG
[[maybe_unused]] static constexpr bool k_enable_state_cache = true;

#define W_RC_BREAK_ON_API_ERROR false

} // namespace erwin