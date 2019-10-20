#pragma once

/*
	Code adapted from the ideas developped in:
		https://blog.molecular-matters.com/2011/07/05/memory-system-part-1/
	Modifications:
		* Using if constexpr and std::is_pod instead of type dispatching to
		  optimize NewArray and DeleteArray for POD types
		* MemoryArena saves allocation size before the front sentinel 
*/

#include <iostream>
#include <iomanip>

#include <cstdint>
#include <type_traits>
#include <algorithm>

#include "core/core.h"

namespace erwin
{

namespace policy
{
class SingleThread
{
public:
	inline void enter() const { }
	inline void leave() const { }
};

// Will work as is with std::mutex, should be
// specialized for other synchronization primitives
template <typename SynchronizationPrimitiveT>
class MultiThread
{
public:
	inline void enter(void) { primitive_.lock(); }
	inline void leave(void) { primitive_.unlock(); }
 
private:
	SynchronizationPrimitiveT primitive_;
};

class NoBoundsChecking
{
public:
	inline void put_sentinel_front(uint8_t* ptr) const   { }
	inline void put_sentinel_back(uint8_t* ptr) const    { }
	inline void check_sentinel_front(uint8_t* ptr) const { }
	inline void check_sentinel_back(uint8_t* ptr) const  { }

	static constexpr size_t SIZE_FRONT = 0;
	static constexpr size_t SIZE_BACK = 0;
};

class SimpleBoundsChecking
{
public:
	inline void put_sentinel_front(uint8_t* ptr) const   { std::fill(ptr, ptr + SIZE_FRONT, 0x0f); }
	inline void put_sentinel_back(uint8_t* ptr) const    { std::fill(ptr, ptr + SIZE_BACK, 0x0f); }
	inline void check_sentinel_front(uint8_t* ptr) const { W_ASSERT(*(uint32_t*)(ptr)==0x0f0f0f0f, "Memory overwrite detected (front)."); }
	inline void check_sentinel_back(uint8_t* ptr) const  { W_ASSERT(*(uint32_t*)(ptr)==0x0f0f0f0f, "Memory overwrite detected (back)."); }

	static constexpr size_t SIZE_FRONT = 4;
	static constexpr size_t SIZE_BACK = 4;
};

class NoMemoryTagging
{
public:
	inline void tag_allocation(uint8_t* ptr, size_t size) const   { }
	inline void tag_deallocation(uint8_t* ptr, size_t size) const { }
};

class NoMemoryTracking
{
public:
	inline void on_allocation(uint8_t*, size_t size, size_t alignment) const { }
	inline void on_deallocation(uint8_t*) const { }
};

}

class HeapArea
{
public:
	HeapArea(size_t size):
	size_(size)
	{
		begin_ = new uint8_t[size_];
	}

	~HeapArea()
	{
		delete[] begin_;
	}

	inline void* begin() { return begin_; }
	inline void* end()   { return begin_+size_+1; }

private:
	size_t size_;
	uint8_t* begin_;
};

template <typename AllocatorT, 
		  typename ThreadPolicyT=policy::SingleThread,
		  typename BoundsCheckerT=policy::NoBoundsChecking,
		  typename MemoryTaggerT=policy::NoMemoryTagging,
		  typename MemoryTrackerT=policy::NoMemoryTracking>
class MemoryArena
{
public:
	typedef uint32_t SIZE_TYPE;

    template <class AreaPolicyT>
    explicit MemoryArena(AreaPolicyT& area):
    allocator_(area.begin(), area.end())
    {

    }

    inline       AllocatorT& get_allocator()       { return allocator_; }
    inline const AllocatorT& get_allocator() const { return allocator_; }

	void* allocate(size_t size, size_t alignment, const char* file, int line)
	{
		// Lock resource if needed
		thread_guard_.enter();

		// Compute size after decoration and allocate
        const size_t naked_size = size;
        const size_t decorated_size = BoundsCheckerT::SIZE_FRONT + sizeof(SIZE_TYPE) + naked_size + BoundsCheckerT::SIZE_BACK;
		uint8_t* begin = static_cast<uint8_t*>(allocator_.allocate(decorated_size, alignment, BoundsCheckerT::SIZE_FRONT + sizeof(SIZE_TYPE)));

		// Put front sentinel for overwrite detection
		bounds_checker_.put_sentinel_front(begin);
		// Save allocation size
		*(reinterpret_cast<SIZE_TYPE*>(begin + BoundsCheckerT::SIZE_FRONT)) = (SIZE_TYPE)decorated_size;

        memory_tagger_.tag_allocation(begin + BoundsCheckerT::SIZE_FRONT + sizeof(SIZE_TYPE), naked_size);
		bounds_checker_.put_sentinel_back(begin + BoundsCheckerT::SIZE_FRONT + sizeof(SIZE_TYPE) + naked_size);
        memory_tracker_.on_allocation(begin, decorated_size, alignment);

		// Unlock resource and return user pointer, just after the front sentinel
		thread_guard_.leave();
		return begin + sizeof(SIZE_TYPE) + BoundsCheckerT::SIZE_FRONT;
	}

	void deallocate(void* ptr)
	{
		thread_guard_.enter();
		uint8_t* begin = (static_cast<uint8_t*>(ptr) - sizeof(SIZE_TYPE) - BoundsCheckerT::SIZE_FRONT);

		// Check the front sentinel before we retrieve the allocation size, just in case
		// the size was corrupted by an overwrite.
        bounds_checker_.check_sentinel_front(begin);
		const SIZE_TYPE decorated_size = *(reinterpret_cast<SIZE_TYPE*>(begin + BoundsCheckerT::SIZE_FRONT));

        bounds_checker_.check_sentinel_back(begin + decorated_size - BoundsCheckerT::SIZE_BACK);
        memory_tracker_.on_deallocation(begin);
        memory_tagger_.tag_deallocation(begin, decorated_size);

		allocator_.deallocate(begin);
		thread_guard_.leave();
	}

private:
	AllocatorT     allocator_;
	ThreadPolicyT  thread_guard_;
	BoundsCheckerT bounds_checker_;
	MemoryTaggerT  memory_tagger_;
	MemoryTrackerT memory_tracker_;
};

template <typename T, class ArenaT>
T* NewArray(ArenaT& arena, size_t N, size_t alignment, const char* file, int line)
{
	if constexpr(std::is_pod<T>::value)
		return static_cast<T*>(arena.allocate(sizeof(T)*N, alignment, file, line));

	typedef typename ArenaT::SIZE_TYPE SIZE_TYPE;
	// Bjarne said union cast was pure evil. Oh well...
	union
	{
		void*      as_void;
		T*         as_T;
		SIZE_TYPE* as_size_t;
	};

	as_void = arena.allocate(sizeof(T)*N + sizeof(SIZE_TYPE), alignment, file, line);

	// new array operator stores the number of instances in the first 4 bytes and
	// returns a pointer to the address right after, we emulate this behavior here,
	// but using arena's size type (which might be larger than 4 bytes).
	*as_size_t++ = N;

	// Construct instances using placement new
	const T* const end = as_T + N;
	while (as_T < end)
		new (as_T++) T;

	// Hand user the pointer to the first instance
	return (as_T - N);
}

// No "placement-delete" in C++, so we define this helper deleter
template <typename T, class ArenaT>
void Delete(T* object, ArenaT& arena)
{
	if constexpr(std::is_pod<T>::value)
	{
    	arena.deallocate(object);
    }
    else
    {
	    object->~T();
	    arena.deallocate(object);
	}
}

template <typename T, class ArenaT>
void DeleteArray(T* object, ArenaT& arena)
{
	if constexpr(std::is_pod<T>::value)
	{
		arena.deallocate(object);
	}
	else
	{
		typedef typename ArenaT::SIZE_TYPE SIZE_TYPE;
		union
		{
			SIZE_TYPE* as_size_t;
			T*         as_T;
		};

		// User pointer points to first instance
		as_T = object;

		// Number of instances stored 4 bytes before first instance
		const SIZE_TYPE N = as_size_t[-1];

		std::cout << N << std::endl;

		// Call instances' destructor in reverse order
		for(uint32_t ii=N; ii>0; --ii)
			as_T[ii-1].~T();

		arena.deallocate(as_size_t-1);
	}
}

template <class T>
struct TypeAndCount { };

template <class T, size_t N>
struct TypeAndCount<T[N]>
{
	typedef T type;
	static constexpr size_t count = N;
};


#define W_NEW( TYPE , ARENA ) new ( ARENA.allocate(sizeof( TYPE ), 0, __FILE__, __LINE__)) TYPE
#define W_NEW_ARRAY( TYPE , ARENA ) NewArray<TypeAndCount< TYPE >::type>( ARENA , TypeAndCount< TYPE >::count, 0, __FILE__, __LINE__)

#define W_NEW_ALIGN( TYPE , ARENA , ALIGNMENT ) new ( ARENA.allocate(sizeof( TYPE ), ALIGNMENT , __FILE__, __LINE__)) TYPE
#define W_NEW_ARRAY_ALIGN( TYPE , ARENA , ALIGNMENT ) NewArray<TypeAndCount< TYPE >::type>( ARENA , TypeAndCount< TYPE >::count, ALIGNMENT , __FILE__, __LINE__)

#define W_DELETE( OBJECT , ARENA ) Delete( OBJECT , ARENA )
#define W_DELETE_ARRAY( OBJECT , ARENA ) DeleteArray( OBJECT , ARENA )

} // namespace erwin