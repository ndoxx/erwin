#include "memory/heap_area.h"
#include "utils/string.h"

namespace erwin
{
namespace memory
{

#ifdef W_DEBUG
void HeapArea::debug_show_content()
{
	size_t b_addr = (size_t)begin_;
	size_t h_addr = (size_t)head_;
	size_t used_mem = h_addr - b_addr;
	float usage = used_mem / float(size_);

	static const float R1 = 204.f; static const float R2 = 255.f;
	static const float G1 = 255.f; static const float G2 = 51.f;
	static const float B1 = 153.f; static const float B2 = 0.f;

	uint8_t R = uint8_t((1.f-usage)*R1 + usage*R2);
	uint8_t G = uint8_t((1.f-usage)*G1 + usage*G2);
	uint8_t B = uint8_t((1.f-usage)*B1 + usage*B2);

	DLOG("memory",1) << "Usage: " << utils::human_size(used_mem) << " / "
					 << utils::human_size(size_) << " (" 
					 << WCC(R,G,B) << 100*usage << WCC(0) << "%)" << std::endl;
	for(auto&& item: items_)
	{
		usage = item.size / float(used_mem);
		R = uint8_t((1.f-usage)*R1 + usage*R2);
		G = uint8_t((1.f-usage)*G1 + usage*G2);
		B = uint8_t((1.f-usage)*B1 + usage*B2);

		std::string name(item.name);
		su::center(name,22);
		DLOGR("memory") << "    0x" << std::hex << item.begin << " [" << WCC(R,G,B) << name << WCC(0) << "] 0x" << item.end 
						<< " s=" << std::dec << utils::human_size(item.size) << std::endl;
	}
}
#endif

} // namespace memory
} // namespace erwin