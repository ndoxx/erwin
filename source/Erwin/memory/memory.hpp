#pragma once

/*
	Code adapted from the ideas developped in:
		https://blog.molecular-matters.com/2011/07/05/memory-system-part-1/
	Modifications:
		* Using if constexpr and std::is_pod instead of type dispatching to
		  optimize NewArray and DeleteArray for POD types
*/

#include <iostream>
#include <iomanip>

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <algorithm>

#include "core/core.h"
#include "debug/logger.h"
#include "memory/memory_utils.h"

// Useful to avoid uninitialized reads with Valgrind during hexdumps
// Disable for retail build
#ifdef W_DEBUG
	#define HEAP_AREA_MEMSET_ENABLED
	#define HEAP_AREA_PADDING_MAGIC
	#define AREA_MEMSET_VALUE 0x00
	#define AREA_PADDING_MARK 0xd0
#endif

namespace erwin
{
namespace memory
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
	inline void put_sentinel_front(uint8_t* ptr) const   { std::fill(ptr, ptr + SIZE_FRONT, 0xf0); }
	inline void put_sentinel_back(uint8_t* ptr) const    { std::fill(ptr, ptr + SIZE_BACK, 0x0f); }
	inline void check_sentinel_front(uint8_t* ptr) const { W_ASSERT(*(uint32_t*)(ptr)==0xf0f0f0f0, "Memory overwrite detected (front)."); }
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
	inline int32_t get_allocation_count() const { return 0; }
	inline void report() const { }
};

class SimpleMemoryTracking
{
public:
	inline void on_allocation(uint8_t*, size_t size, size_t alignment) { ++num_allocs_; }
	inline void on_deallocation(uint8_t*) { --num_allocs_; }
	inline int32_t get_allocation_count() const { return num_allocs_; }
	inline void report() const
	{
		if(num_allocs_)
		{
			DLOGE("memory") << "[MemoryTracker] Alloc-dealloc mismatch: " << num_allocs_ << std::endl;
		}
	}

private:
	int32_t num_allocs_ = 0;
};

}

class HeapArea
{
public:
	HeapArea() = default;
	HeapArea(size_t size)
	{
		init(size);
	}

	~HeapArea()
	{
		delete[] begin_;
	}

	inline bool init(size_t size)
	{
		DLOG("memory",1) << WCC('i') << "[HeapArea]" << WCC(0) << " Initializing:" << std::endl;
		size_ = size;
		begin_ = new uint8_t[size_];
		head_ = begin_;
#ifdef HEAP_AREA_MEMSET_ENABLED
		memset(begin_, AREA_MEMSET_VALUE, size_);
#endif
		DLOGI << "Size:  " << WCC('v') << size_ << WCC(0) << "B" << std::endl;
		DLOGI << "Begin: 0x" << std::hex << uint64_t(begin_) << std::dec << std::endl;
		return true;
	}

	inline void* begin() { return begin_; }
	inline void* end()   { return begin_+size_+1; }
	inline std::pair<void*,void*> range() { return {begin(), end()}; }

	// Get a range of pointers to a memory block within area, and advance head
	inline std::pair<void*,void*> require_block(size_t size)
	{
		// Page align returned block to avoid false sharing if multiple threads access this area
        size_t padding = utils::alignment_padding((std::size_t)(head_), 64);
		W_ASSERT(head_ + size + padding < end(), "[HeapArea] Out of memory!");

    	// Mark padding area
#ifdef HEAP_AREA_PADDING_MAGIC
    	std::fill(head_, head_ + padding, AREA_PADDING_MARK);
#endif

		std::pair<void*,void*> range = {head_ + padding, head_ + padding + size + 1};

		DLOG("memory",1) << WCC('i') << "[HeapArea]" << WCC(0) << " allocated aligned block:" << std::endl;
		DLOGI << "Size:      "   << WCC('v') << size                                                 << WCC(0) << "B" << std::endl;
		DLOGI << "Padding:   "   << WCC('v') << padding                                              << WCC(0) << "B" << std::endl;
		DLOGI << "Remaining: "   << WCC('v') << uint64_t((uint8_t*)(end())-(head_ + size + padding)) << WCC(0) << "B" << std::endl;
		DLOGI << "Address:   0x" << std::hex << uint64_t(head_ + padding)                            << std::dec << std::endl;

		head_ += size + padding;
		return range;
	}

	template <typename PoolT>
	inline void* require_pool_block(size_t element_size, size_t max_count)
	{
		size_t decorated_size = element_size + PoolT::DECORATION_SIZE;
		size_t pool_size = max_count * decorated_size;
		auto block = require_block(pool_size);
		return block.first;
	}

#ifdef W_DEBUG
	inline void debug_hex_dump(std::ostream& stream, size_t size=0)
	{
		if(size == 0)
			size = size_t(head_-begin_);
    	memory::hex_dump(stream, begin_, size, "HEX DUMP");
	}
#endif

private:
	size_t size_;
	uint8_t* begin_;
	uint8_t* head_;
};

// TODO: OPTIMIZE - MemoryArena could be partially specialized for PoolAllocator
//                  so as to avoid writing the allocation size before each element 
template <typename AllocatorT, 
		  typename ThreadPolicyT=policy::SingleThread,
		  typename BoundsCheckerT=policy::NoBoundsChecking,
		  typename MemoryTaggerT=policy::NoMemoryTagging,
		  typename MemoryTrackerT=policy::NoMemoryTracking>
class MemoryArena
{
public:
	typedef uint32_t SIZE_TYPE; // BUG#4 if set to size_t/uint64_t, probably because of endianness
	// Size of bookkeeping data before user pointer
	static constexpr uint32_t BK_FRONT_SIZE = BoundsCheckerT::SIZE_FRONT
											+ sizeof(SIZE_TYPE);
	static constexpr uint32_t DECORATION_SIZE = BK_FRONT_SIZE
											  + BoundsCheckerT::SIZE_BACK;

	// Wrap the allocator ctor
    template <typename... ArgsT>
    explicit MemoryArena(ArgsT&&... args):
    allocator_(std::forward<ArgsT>(args)...)
    {

    }

    ~MemoryArena()
    {
    	memory_tracker_.report();
    }

    inline       AllocatorT& get_allocator()       { return allocator_; }
    inline const AllocatorT& get_allocator() const { return allocator_; }

    inline void set_debug_name(const std::string& name) { debug_name_ = name; }

	void* allocate(size_t size, size_t alignment, size_t offset, const char* file, int line)
	{
		// Lock resource
		thread_guard_.enter();

		// Compute size after decoration and allocate
        const size_t decorated_size = DECORATION_SIZE + size;
		const size_t user_offset = BK_FRONT_SIZE + offset;

		uint8_t* begin = static_cast<uint8_t*>(allocator_.allocate(decorated_size, alignment, user_offset));

#ifdef W_DEBUG
		if(begin == nullptr)
		{
			DLOGF("memory") << "[Arena] \"" << debug_name_ << "\": Out of memory." << std::endl;
			W_ASSERT(false, "Allocation failed.");
		}
#endif

		uint8_t* current = begin;

		// Put front sentinel for overwrite detection
		bounds_checker_.put_sentinel_front(current);
		current += BoundsCheckerT::SIZE_FRONT;

		// Save allocation size
		*(reinterpret_cast<SIZE_TYPE*>(current)) = (SIZE_TYPE)decorated_size;
		current += sizeof(SIZE_TYPE);

		// More bookkeeping
        memory_tagger_.tag_allocation(current, size);
		bounds_checker_.put_sentinel_back(current + size);
        memory_tracker_.on_allocation(begin, decorated_size, alignment);

		// Unlock resource and return user pointer
		thread_guard_.leave();
		return current;
	}

	void deallocate(void* ptr)
	{
		thread_guard_.enter();
		// Take care to jump further back if non-POD array deallocation, because we also stored the number of instances
		uint8_t* begin = static_cast<uint8_t*>(ptr) - BK_FRONT_SIZE;

		// Check the front sentinel before we retrieve the allocation size, just in case
		// the size was corrupted by an overwrite.
        bounds_checker_.check_sentinel_front(begin);
		const SIZE_TYPE decorated_size = *(reinterpret_cast<SIZE_TYPE*>(begin + BoundsCheckerT::SIZE_FRONT));

		// Check that everything went ok
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

	std::string debug_name_;
};

template <typename T, class ArenaT>
T* NewArray(ArenaT& arena, size_t N, size_t alignment, const char* file, int line)
{
	if constexpr(std::is_pod<T>::value)
	{
		return static_cast<T*>(arena.allocate(sizeof(T)*N, alignment, 0, file, line));
	}
	else
	{
		// new[] operator stores the number of instances in the first 4 bytes and
		// returns a pointer to the address right after, we emulate this behavior here.
		union
		{
			uint32_t* as_uint;
			T*        as_T;
		};
		as_uint = static_cast<uint32_t*>(arena.allocate(sizeof(uint32_t) + sizeof(T)*N, alignment, sizeof(uint32_t), file, line));
		*(as_uint++) = (uint32_t)N;

		// Construct instances using placement new
		const T* const end = as_T + N;
		while (as_T < end)
			new (as_T++) T;

		// Hand user the pointer to the first instance
		return (as_T - N);
	}
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
		union
		{
			uint32_t* as_uint;
			T*        as_T;
		};
		// User pointer points to first instance
		as_T = object;
		// Number of instances stored 4 bytes before first instance
		const uint32_t N = as_uint[-1];

		// Call instances' destructor in reverse order
		for(uint32_t ii=N; ii>0; --ii)
			as_T[ii-1].~T();

		// Arena's deallocate() expects a pointer 4 bytes before actual user pointer
		arena.deallocate(as_uint-1);
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


template <typename ThreadPolicyT=policy::SingleThread>
class LinearBuffer
{
public:
	LinearBuffer(std::pair<void*,void*> ptr_range):
	begin_(static_cast<uint8_t*>(ptr_range.first)),
	end_(static_cast<uint8_t*>(ptr_range.second)),
	head_(begin_)
	{

	}

#ifdef W_DEBUG
    inline void set_debug_name(const std::string& name) { debug_name_ = name; }
#endif

	inline void write(void const* source, std::size_t size)
	{
#ifdef W_DEBUG
		if(head_ + size >= end_)
		{
			DLOGF("memory") << "[LinearBuffer] \"" << debug_name_ << "\": Data buffer overwrite!" << std::endl;
		}
#endif
		W_ASSERT(head_ + size < end_, "[LinearBuffer] Data buffer overwrite!");
		memcpy(head_, source, size);
		head_ += size;
	}
	inline void read(void* destination, std::size_t size)
	{
#ifdef W_DEBUG
		if(head_ + size >= end_)
		{
			DLOGF("memory") << "[LinearBuffer] \"" << debug_name_ << "\": Data buffer overread!" << std::endl;
		}
#endif
		W_ASSERT(head_ + size < end_, "[LinearBuffer] Data buffer overread!");
		memcpy(destination, head_, size);
		head_ += size;
	}
	template <typename T>
	inline void write(T* source)     { write(source, sizeof(T)); }
	template <typename T>
	inline void read(T* destination) { read(destination, sizeof(T)); }
	inline void write_str(const std::string& str)
	{
		uint32_t str_size = str.size();
		write(&str_size, sizeof(uint32_t));
		write(str.data(), str_size);
	}
	inline void read_str(std::string& str)
	{
		uint32_t str_size;
		read(&str_size, sizeof(uint32_t));
		str.resize(str_size);
		read(str.data(), str_size);
	}

	inline void reset()                 { head_ = begin_; }
	inline uint8_t* head()              { return head_; }
	inline uint8_t* begin()             { return begin_; }
	inline const uint8_t* begin() const { return begin_; }
	inline void seek(void* ptr)         { head_ = static_cast<uint8_t*>(ptr); }

private:
	uint8_t* begin_;
	uint8_t* end_;
	uint8_t* head_;

#ifdef W_DEBUG
	std::string debug_name_;
#endif
};

} // namespace memory

// User literals for bytes multiples
constexpr size_t operator"" _B(unsigned long long size)  { return size; }
constexpr size_t operator"" _kB(unsigned long long size) { return 1024*size; }
constexpr size_t operator"" _MB(unsigned long long size) { return 1048576*size; }
constexpr size_t operator"" _GB(unsigned long long size) { return 1073741824*size; }

#define W_NEW( TYPE , ARENA ) new ( ARENA.allocate(sizeof( TYPE ), 0, 0, __FILE__, __LINE__)) TYPE
#define W_NEW_ARRAY( TYPE , ARENA ) memory::NewArray<memory::TypeAndCount< TYPE >::type>( ARENA , memory::TypeAndCount< TYPE >::count, 0, __FILE__, __LINE__)
#define W_NEW_ARRAY_DYNAMIC( TYPE , COUNT , ARENA ) memory::NewArray< TYPE >( ARENA , COUNT , 0, __FILE__, __LINE__)

#define W_NEW_ALIGN( TYPE , ARENA , ALIGNMENT ) new ( ARENA.allocate(sizeof( TYPE ), ALIGNMENT , 0, __FILE__, __LINE__)) TYPE
#define W_NEW_ARRAY_ALIGN( TYPE , ARENA , ALIGNMENT ) memory::NewArray<memory::TypeAndCount< TYPE >::type>( ARENA , memory::TypeAndCount< TYPE >::count, ALIGNMENT , __FILE__, __LINE__)
#define W_NEW_ARRAY_DYNAMIC_ALIGN( TYPE , COUNT , ARENA , ALIGNMENT ) memory::NewArray< TYPE >( ARENA , COUNT , ALIGNMENT , __FILE__, __LINE__)

#define W_DELETE( OBJECT , ARENA ) memory::Delete( OBJECT , ARENA )
#define W_DELETE_ARRAY( OBJECT , ARENA ) memory::DeleteArray( OBJECT , ARENA )

} // namespace erwin