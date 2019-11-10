#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <bitset>

// using namespace erwin;

struct SortKey
{
	enum class Order: uint8_t
	{
		ByShader,
		ByDepthDescending,
		ByDepthAscending,
		Sequential
	};

	uint64_t encode(SortKey::Order type) const;

						      // -- dependencies --     -- meaning --
	uint8_t view = 0;         // queue global state?	layer / viewport id
	uint8_t transparency = 0; // queue type 			blending type: opaque / transparent
	uint8_t shader = 0;       // command data / type    could mean "material ID" when I have a material system
	uint32_t depth = 0;       // command data / type 	depth mantissa
	uint32_t sequence = 0;    // command data / type 	for commands to be dispatched sequentially
};

constexpr uint8_t  k_view_bits       = 8;
constexpr uint8_t  k_draw_type_bits  = 2;
constexpr uint8_t  k_transp_bits     = 1;
constexpr uint8_t  k_shader_bits     = 8;
constexpr uint8_t  k_depth_bits      = 32;
constexpr uint8_t  k_seq_bits        = 32;
constexpr uint64_t k_view_shift      = uint8_t(64)       - k_view_bits;
constexpr uint64_t k_draw_type_shift = k_view_shift  - k_draw_type_bits;
constexpr uint64_t k_1_transp_shift  = k_draw_type_shift - k_transp_bits;
constexpr uint64_t k_1_shader_shift  = k_1_transp_shift  - k_shader_bits;
constexpr uint64_t k_1_depth_shift   = k_1_shader_shift  - k_depth_bits;
constexpr uint64_t k_2_depth_shift   = k_draw_type_shift - k_depth_bits;
constexpr uint64_t k_2_transp_shift  = k_2_depth_shift   - k_transp_bits;
constexpr uint64_t k_2_shader_shift  = k_2_transp_shift  - k_shader_bits;
constexpr uint64_t k_3_seq_shift     = k_draw_type_shift - k_seq_bits;
constexpr uint64_t k_3_transp_shift  = k_3_seq_shift     - k_transp_bits;
constexpr uint64_t k_3_shader_shift  = k_3_transp_shift  - k_shader_bits;
constexpr uint64_t k_view_mask       = uint64_t(0x0000000f) << k_view_shift;
constexpr uint64_t k_draw_type_mask  = uint64_t(0x00000003) << k_draw_type_shift;
constexpr uint64_t k_1_transp_mask   = uint64_t(0x00000001) << k_1_transp_shift;
constexpr uint64_t k_1_shader_mask   = uint64_t(0x000000ff) << k_1_shader_shift;
constexpr uint64_t k_1_depth_mask    = uint64_t(0xffffffff) << k_1_depth_shift;
constexpr uint64_t k_2_depth_mask    = uint64_t(0xffffffff) << k_2_depth_shift;
constexpr uint64_t k_2_transp_mask   = uint64_t(0x00000001) << k_2_transp_shift;
constexpr uint64_t k_2_shader_mask   = uint64_t(0x000000ff) << k_2_shader_shift;
constexpr uint64_t k_3_seq_mask      = uint64_t(0xffffffff) << k_3_seq_shift;
constexpr uint64_t k_3_transp_mask   = uint64_t(0x00000001) << k_3_transp_shift;
constexpr uint64_t k_3_shader_mask   = uint64_t(0x000000ff) << k_3_shader_shift;

uint64_t SortKey::encode(SortKey::Order type) const
{
	uint64_t head = ((uint64_t(view)         << k_view_shift     ) & k_view_mask)
				  | ((uint64_t(0)	   	     << k_draw_type_shift) & k_draw_type_mask);

	uint64_t body = 0;
	switch(type)
	{
		case SortKey::Order::ByShader:
		{
			body |= ((uint64_t(transparency) << k_1_transp_shift) & k_1_transp_mask)
				 |  ((uint64_t(shader)       << k_1_shader_shift) & k_1_shader_mask)
				 |  ((uint64_t(depth)        << k_1_depth_shift)  & k_1_depth_mask);
			break;
		}
		case SortKey::Order::ByDepthDescending:
		{
			body |= ((uint64_t(~depth)       << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(transparency) << k_2_transp_shift) & k_2_transp_mask)
				 |  ((uint64_t(shader)       << k_2_shader_shift) & k_2_shader_mask);
			break;
		}
		case SortKey::Order::ByDepthAscending:
		{
			body |= ((uint64_t(depth)        << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(transparency) << k_2_transp_shift) & k_2_transp_mask)
				 |  ((uint64_t(shader)       << k_2_shader_shift) & k_2_shader_mask);
			break;
		}
		case SortKey::Order::Sequential:
		{
			body |= ((uint64_t(sequence)     << k_3_seq_shift)    & k_3_seq_mask)
				 |  ((uint64_t(transparency) << k_3_transp_shift) & k_3_transp_mask)
				 |  ((uint64_t(shader)       << k_3_shader_shift) & k_3_shader_mask);
			break;
		}
	}

	return head | body;
}

inline void set_key(SortKey& key, float depth)
{
	// TODO: Normalize depth and extract 24b mantissa
	key.depth = *((uint32_t*)(&depth));
}

int main(int argc, char** argv)
{
	std::vector<uint64_t> keys;
	for(int ii=0; ii<10; ++ii)
	{
		SortKey k;
		set_key(k, -ii/10.f);
		uint64_t ke = k.encode(SortKey::Order::ByDepthDescending);
		keys.push_back(ke);
		std::cout << ke << std::endl;
	}

	return 0;
}
