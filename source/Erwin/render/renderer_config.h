#pragma once

#include <cstdint>
// #define W_PROFILE_RENDER

namespace erwin
{

#ifdef W_PROFILE_RENDER
	#include "debug/instrumentor.h"
	#define W_PROFILE_RENDER_SCOPE(name) erwin::InstrumentationTimer timer##__LINE__( name );
	#define W_PROFILE_RENDER_FUNCTION() W_PROFILE_RENDER_SCOPE( __PRETTY_FUNCTION__ )
#else
	#define W_PROFILE_RENDER_SCOPE(name)
	#define W_PROFILE_RENDER_FUNCTION()
#endif

#define W_RC_PROFILE_DRAW_CALLS true

// Maximum amount of texture slots per draw call
static constexpr uint32_t k_max_texture_slots = 32;
// Maximum amount of cubemap slots per draw call
static constexpr uint32_t k_max_cubemap_slots = 8;
// Maximum amount of render commands per frame, per command buffer
static constexpr uint32_t k_max_render_commands = 2048;
// Maximum amount of draw calls per frame
static constexpr uint32_t k_max_draw_calls = 8192;
// Maximum amount of dependencies per draw call
static constexpr uint32_t k_max_draw_call_dependencies = 8;

// Maximum amount of handles for every object managed by the main renderer
// Default is 256, but the HANDLE_DECLARATION() macro overrides this setting
template<typename HandleT> static constexpr uint32_t k_max_handles = 256;

} // namespace erwin