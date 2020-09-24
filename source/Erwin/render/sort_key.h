#pragma once

#include <cstdint>
#include <limits>

#include "render/render_state.h"

namespace erwin
{

struct SortKey
{
	// Policy for key sorting
	enum class Order: uint8_t
	{
		ByShader,			// Keys are sorted by shader in priority, then by depth
		ByDepthDescending,	// Keys are sorted by increasing clip depth first, then by shader
		ByDepthAscending,	// Keys are sorted by decreasing clip depth first, then by shader
		Sequential 			// Keys are sorted by a 32-bits sequence number
	};

	static constexpr uint64_t k_skip = std::numeric_limits<uint64_t>::max();

	// Encode key structure into a 64 bits number (the actual sorting key)
	uint64_t encode() const;

	inline void set_depth(float _depth, uint8_t layer_id, uint64_t state_flags, ShaderHandle shader_handle, uint8_t _sub_sequence=0)
	{
		W_ASSERT(shader_handle.index()<256, "Shader index out of bounds in shader sorting key section.");
		view         = uint16_t(uint16_t(layer_id)<<8);
		view        |= uint8_t((state_flags & k_framebuffer_mask) >> k_framebuffer_shift);
		shader       = uint8_t(shader_handle.index());
		sub_sequence = _sub_sequence;
		depth        = uint32_t(glm::clamp(std::fabs(_depth), 0.f, 1.f) * 0x00ffffff);
		blending     = RenderState::is_transparent(state_flags);
		order        = blending ? SortKey::Order::ByDepthAscending : SortKey::Order::ByDepthDescending;
	}

	inline void set_sequence(uint32_t _sequence, uint8_t layer_id, ShaderHandle shader_handle, uint8_t _sub_sequence=0)
	{
		W_ASSERT(shader_handle.index()<256, "Shader index out of bounds in shader sorting key section.");
		view         = uint16_t(uint16_t(layer_id)<<8);
		shader       = uint8_t(shader_handle.index());
		sub_sequence = _sub_sequence;
		sequence     = _sequence;
		blending     = false;
		order        = SortKey::Order::Sequential;
	}

	uint16_t view = 0;        // [layer id | framebuffer id] for depth order, just layer id for sequential order
	uint8_t shader = 0;       // shader id to allow grouping by shader
	uint8_t sub_sequence = 0; // allows draw command chaining even in depth mode
	uint32_t depth = 0;       // 24 bits clamped absolute normalized depth
	uint32_t sequence = 0;    // for commands to be dispatched sequentially
	bool blending = false;    // affects the draw_type bits
	SortKey::Order order;     // impacts how this key will be encoded
};

} // namespace erwin