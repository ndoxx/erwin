#include "render/sort_key.h"

namespace erwin
{

constexpr uint8_t  k_view_bits       = 16;
constexpr uint8_t  k_draw_type_bits  = 2;
constexpr uint8_t  k_shader_bits     = 8;
constexpr uint8_t  k_depth_bits      = 24;
constexpr uint8_t  k_seq_bits        = 24;
constexpr uint8_t  k_subseq_bits     = 8;
constexpr uint64_t k_view_shift      = uint8_t(64)        - k_view_bits;
constexpr uint64_t k_draw_type_shift = k_view_shift       - k_draw_type_bits;
constexpr uint64_t k_1_shader_shift  = k_draw_type_shift  - k_shader_bits;
constexpr uint64_t k_1_depth_shift   = k_1_shader_shift   - k_depth_bits;
constexpr uint64_t k_1_subseq_shift  = k_1_depth_shift    - k_subseq_bits;
constexpr uint64_t k_2_depth_shift   = k_draw_type_shift  - k_depth_bits;
constexpr uint64_t k_2_shader_shift  = k_2_depth_shift    - k_shader_bits;
constexpr uint64_t k_2_subseq_shift  = k_2_shader_shift   - k_subseq_bits;
constexpr uint64_t k_3_seq_shift     = k_draw_type_shift  - k_seq_bits;
constexpr uint64_t k_3_shader_shift  = k_3_seq_shift      - k_shader_bits;
constexpr uint64_t k_3_subseq_shift  = k_3_shader_shift   - k_subseq_bits;
constexpr uint64_t k_view_mask       = uint64_t(0x0000ffff) << k_view_shift;
constexpr uint64_t k_draw_type_mask  = uint64_t(0x00000003) << k_draw_type_shift;
constexpr uint64_t k_1_shader_mask   = uint64_t(0x000000ff) << k_1_shader_shift;
constexpr uint64_t k_1_depth_mask    = uint64_t(0x00ffffff) << k_1_depth_shift;
constexpr uint64_t k_1_subseq_mask   = uint64_t(0x000000ff) << k_1_subseq_shift;
constexpr uint64_t k_2_depth_mask    = uint64_t(0x00ffffff) << k_2_depth_shift;
constexpr uint64_t k_2_shader_mask   = uint64_t(0x000000ff) << k_2_shader_shift;
constexpr uint64_t k_2_subseq_mask   = uint64_t(0x000000ff) << k_2_subseq_shift;
constexpr uint64_t k_3_seq_mask      = uint64_t(0x00ffffff) << k_3_seq_shift;
constexpr uint64_t k_3_shader_mask   = uint64_t(0x000000ff) << k_3_shader_shift;
constexpr uint64_t k_3_subseq_mask   = uint64_t(0x000000ff) << k_3_subseq_shift;

uint64_t SortKey::encode() const
{
	uint64_t head = ((uint64_t(view)     << k_view_shift     ) & k_view_mask)
				  | ((uint64_t(blending) << k_draw_type_shift) & k_draw_type_mask);

	uint64_t body = 0;
	switch(order)
	{
		case SortKey::Order::ByShader:
		{
			body |= ((uint64_t(shader)       << k_1_shader_shift) & k_1_shader_mask)
				 |  ((uint64_t(depth)        << k_1_depth_shift)  & k_1_depth_mask)
				 |  ((uint64_t(sub_sequence) << k_1_subseq_shift) & k_1_subseq_mask);
			break;
		}
		case SortKey::Order::ByDepthDescending:
		{
			body |= ((uint64_t(depth)        << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(shader)       << k_2_shader_shift) & k_2_shader_mask)
				 |  ((uint64_t(sub_sequence) << k_2_subseq_shift) & k_2_subseq_mask);
			break;
		}
		case SortKey::Order::ByDepthAscending:
		{
			body |= ((uint64_t(~depth)       << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(shader)       << k_2_shader_shift) & k_2_shader_mask)
				 |  ((uint64_t(sub_sequence) << k_2_subseq_shift) & k_2_subseq_mask);
			break;
		}
		case SortKey::Order::Sequential:
		{
			body |= ((uint64_t(sequence)     << k_3_seq_shift)    & k_3_seq_mask)
				 |  ((uint64_t(shader)       << k_3_shader_shift) & k_3_shader_mask)
				 |  ((uint64_t(sub_sequence) << k_3_subseq_shift) & k_3_subseq_mask);
			break;
		}
	}

	return head | body;
}

} // namespace erwin