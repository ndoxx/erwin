#pragma once

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

// Maximum amount of texture slots per draw call
static constexpr uint32_t k_max_texture_slots = 4;
// Maximum amount of render commands per frame, per command buffer
static constexpr uint32_t k_max_render_commands = 8192;
// Maximum amount of handles for every object managed by the main renderer
// TODO: each object should have its own amount
static constexpr uint32_t k_max_render_handles = 128;

} // namespace erwin