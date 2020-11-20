#include "memory/heap_area.h"
#include <kibble/string/string.h>

namespace erwin
{
namespace memory
{

#ifdef W_DEBUG
void HeapArea::debug_show_content()
{
	size_t b_addr = reinterpret_cast<size_t>(begin_);
	size_t h_addr = reinterpret_cast<size_t>(head_);
	size_t used_mem = h_addr - b_addr;
	float usage = float(used_mem) / float(size_);

	static const float R1 = 204.f; static const float R2 = 255.f;
	static const float G1 = 255.f; static const float G2 = 51.f;
	static const float B1 = 153.f; static const float B2 = 0.f;

	uint8_t R = uint8_t((1.f-usage)*R1 + usage*R2);
	uint8_t G = uint8_t((1.f-usage)*G1 + usage*G2);
	uint8_t B = uint8_t((1.f-usage)*B1 + usage*B2);

	KLOG("memory",1) << "Usage: " << utils::human_size(used_mem) << " / "
					 << utils::human_size(size_) << " (" 
					 << kb::WCC(R,G,B) << 100*usage << kb::WCC(0) << "%)" << std::endl;
	for(auto&& item: items_)
	{
		usage = float(item.size) / float(used_mem);
		R = uint8_t((1.f-usage)*R1 + usage*R2);
		G = uint8_t((1.f-usage)*G1 + usage*G2);
		B = uint8_t((1.f-usage)*B1 + usage*B2);

		std::string name(item.name);
		kb::su::center(name,22);
		KLOGR("memory") << "    0x" << std::hex << item.begin << " [" << kb::WCC(R,G,B) << name << kb::WCC(0) << "] 0x" << item.end 
						<< " s=" << std::dec << utils::human_size(item.size) << std::endl;
	}
}
#endif

} // namespace memory
} // namespace erwin