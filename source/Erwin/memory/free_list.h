#pragma once

#include <cstdint>

namespace erwin
{
namespace memory
{

class Freelist
{
public:
	Freelist() = default;
	Freelist(void* begin, std::size_t element_size, std::size_t max_elements, std::size_t alignment, std::size_t offset);

	void init(void* begin, std::size_t element_size, std::size_t max_elements, std::size_t alignment, std::size_t offset);

	inline void* acquire()
	{
		// Return null if no more entry left
		if(next_ == nullptr)
			return nullptr;

		// Obtain one element from the head of the free list
		Freelist* head = next_;
		next_ = head->next_;
		return head;
	}

	inline void release(void* ptr)
	{
		// Put the returned element at the head of the free list
		Freelist* head = static_cast<Freelist*>(ptr);
		head->next_ = next_;
		next_ = head;
	}
 
#ifdef W_DEBUG
	inline void* next(void* ptr)
	{
		return static_cast<Freelist*>(ptr)->next_;
	}
#endif
 
private:
  Freelist* next_;
};

} // namespace memory
} // namespace erwin