#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>

#include "catch2/catch.hpp"
#include "memory/memory.hpp"
#include "memory/linear_allocator.h"
#include "memory/memory_utils.h"
#include "debug/logger_common.h"

using namespace erwin;

struct POD
{
	uint32_t a;
	uint32_t b;
	uint64_t c;
};

struct NonPOD
{
	NonPOD(uint32_t a, uint32_t b):
	a(a), b(b), c(0x42424242)
	{
		data = new uint32_t[a];
		for(int ii=0; ii<a; ++ii)
			data[ii] = b;
	}

	NonPOD():NonPOD(4,0xB16B00B5) { }

	~NonPOD()
	{
		delete[] data;
	}

	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t* data;
};

typedef memory::MemoryArena<memory::LinearAllocator, 
		    				memory::policy::SingleThread, 
		    				memory::policy::SimpleBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::SimpleMemoryTracking> LinArena;

class LinArenaFixture
{
public:
	typedef typename LinArena::SIZE_TYPE SIZE_TYPE;
	
	LinArenaFixture():
	area(1_kB),
	arena(area.begin(), area.end())
	{

	}

protected:
	memory::HeapArea area;
	LinArena arena;
};

TEST_CASE_METHOD(LinArenaFixture, "new POD non-aligned", "[mem]")
{
	POD* some_pod = W_NEW(POD, arena);
	some_pod->a = 0x42424242;
	some_pod->b = 0xB16B00B5;
	some_pod->c = 0xD0D0DADAD0D0DADA;
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(some_pod) - LinArena::BK_FRONT_SIZE, sizeof(POD) + LinArena::DECORATION_SIZE);
	
	// Arena should write the complete allocation size just before user pointer
	REQUIRE(*(reinterpret_cast<SIZE_TYPE*>(some_pod)-1) == sizeof(POD) + LinArena::DECORATION_SIZE);

	W_DELETE(some_pod, arena);
}

TEST_CASE_METHOD(LinArenaFixture, "new POD aligned", "[mem]")
{
	POD* some_pod = W_NEW_ALIGN(POD, arena, 16);
	some_pod->a = 0x42424242;
	some_pod->b = 0xB16B00B5;
	some_pod->c = 0xD0D0DADAD0D0DADA;
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(some_pod) - LinArena::BK_FRONT_SIZE, sizeof(POD) + LinArena::DECORATION_SIZE);

	// Check that returned address is correctly 16 bytes aligned
	REQUIRE(size_t(some_pod)%16==0);
	// Arena should write the complete allocation size just before user pointer
	REQUIRE(*(reinterpret_cast<SIZE_TYPE*>(some_pod)-1) == sizeof(POD) + LinArena::DECORATION_SIZE);

	W_DELETE(some_pod, arena);
}

TEST_CASE_METHOD(LinArenaFixture, "multiple alignments test", "[mem]")
{
	for(uint32_t ALIGNMENT=2; ALIGNMENT<=128; ALIGNMENT*=2)
	{
		POD* some_pod = W_NEW_ALIGN(POD, arena, ALIGNMENT);
		some_pod->a = 0xB16B00B5;
		REQUIRE(size_t(some_pod)%ALIGNMENT==0);
		W_DELETE(some_pod, arena);
	}
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(area.begin()), 512);
}

TEST_CASE_METHOD(LinArenaFixture, "new POD array non-aligned", "[mem]")
{
	const uint32_t N = 10;

	POD* pod_array = W_NEW_ARRAY(POD[N], arena);
	for(int ii=0; ii<N; ++ii)
	{
		pod_array[ii].a = 0x42424242;
		pod_array[ii].b = 0xB16B00B5;
		pod_array[ii].c = 0xD0D0DADAD0D0DADA;
	}
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(pod_array) - LinArena::BK_FRONT_SIZE - 4, N*sizeof(POD) + LinArena::DECORATION_SIZE + 4);
	
	// Arena should write the complete allocation size just before the user pointer
	REQUIRE(*(reinterpret_cast<SIZE_TYPE*>(pod_array)-1) == N*sizeof(POD) + LinArena::DECORATION_SIZE);

	W_DELETE_ARRAY(pod_array, arena);
}

TEST_CASE_METHOD(LinArenaFixture, "new POD array aligned", "[mem]")
{
	const uint32_t N = 10;

	POD* pod_array = W_NEW_ARRAY_ALIGN(POD[N], arena, 32);
	for(int ii=0; ii<N; ++ii)
	{
		pod_array[ii].a = 0x42424242;
		pod_array[ii].b = 0xB16B00B5;
		pod_array[ii].c = 0xD0D0DADAD0D0DADA;
	}
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(pod_array) - LinArena::BK_FRONT_SIZE - 4, N*sizeof(POD) + LinArena::DECORATION_SIZE + 4);
	
	// Check that returned address is correctly 32 bytes aligned
	REQUIRE(size_t(pod_array)%32==0);
	// Arena should write the complete allocation size just before the user pointer
	REQUIRE(*(reinterpret_cast<SIZE_TYPE*>(pod_array)-1) == N*sizeof(POD) + LinArena::DECORATION_SIZE);

	W_DELETE_ARRAY(pod_array, arena);
}

TEST_CASE_METHOD(LinArenaFixture, "new non-POD non-aligned", "[mem]")
{
	NonPOD* some_non_pod = W_NEW(NonPOD, arena)(10,8);
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(some_non_pod) - LinArena::BK_FRONT_SIZE, sizeof(NonPOD) + LinArena::DECORATION_SIZE);
	
	// Arena should write the complete allocation size just before user pointer
	REQUIRE(*(reinterpret_cast<SIZE_TYPE*>(some_non_pod)-1) == sizeof(NonPOD) + LinArena::DECORATION_SIZE);

	W_DELETE(some_non_pod, arena);
}

TEST_CASE_METHOD(LinArenaFixture, "new non-POD aligned", "[mem]")
{
	NonPOD* some_non_pod = W_NEW_ALIGN(NonPOD, arena, 32)(10,8);
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(some_non_pod) - LinArena::BK_FRONT_SIZE, sizeof(NonPOD) + LinArena::DECORATION_SIZE);
	
	// Check that returned address is correctly 32 bytes aligned
	REQUIRE(size_t(some_non_pod)%32==0);
	// Arena should write the complete allocation size just before user pointer
	REQUIRE(*(reinterpret_cast<SIZE_TYPE*>(some_non_pod)-1) == sizeof(NonPOD) + LinArena::DECORATION_SIZE);

	W_DELETE(some_non_pod, arena);
}

TEST_CASE_METHOD(LinArenaFixture, "new non-POD array non-aligned", "[mem]")
{
	const uint32_t N = 4;

	NonPOD* non_pod_array = W_NEW_ARRAY(NonPOD[N], arena);
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(non_pod_array) - LinArena::BK_FRONT_SIZE - 4, N*sizeof(NonPOD) + LinArena::DECORATION_SIZE + 4);
	
	// Arena should write the number of instances before the returned user pointer
	REQUIRE(*(reinterpret_cast<SIZE_TYPE*>(non_pod_array)-1) == N);
	// Arena should write the complete allocation size just before the number of instances
	REQUIRE(*(reinterpret_cast<SIZE_TYPE*>(non_pod_array)-2) == N*sizeof(NonPOD) + LinArena::DECORATION_SIZE + sizeof(SIZE_TYPE));

	W_DELETE_ARRAY(non_pod_array, arena);
}

TEST_CASE_METHOD(LinArenaFixture, "new non-POD array aligned", "[mem]")
{
	const uint32_t N = 4;

	NonPOD* non_pod_array = W_NEW_ARRAY_ALIGN(NonPOD[N], arena, 16);
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(non_pod_array) - LinArena::BK_FRONT_SIZE - 4, N*sizeof(NonPOD) + LinArena::DECORATION_SIZE + 4);
	
	// Check that returned address is correctly 16 bytes aligned
	REQUIRE(size_t(non_pod_array)%16==0);
	// Arena should write the number of instances before the returned user pointer
	REQUIRE(*(reinterpret_cast<SIZE_TYPE*>(non_pod_array)-1) == N);
	// Arena should write the complete allocation size just before the number of instances
	REQUIRE(*(reinterpret_cast<SIZE_TYPE*>(non_pod_array)-2) == N*sizeof(NonPOD) + LinArena::DECORATION_SIZE + sizeof(SIZE_TYPE));

	W_DELETE_ARRAY(non_pod_array, arena);
}

TEST_CASE_METHOD(LinArenaFixture, "multiple allocations", "[mem]")
{
	for(int ii=0; ii<10; ++ii)
	{
		if(ii%3)
		{
			POD* some_pod = W_NEW_ALIGN(POD, arena, 16);
			some_pod->a = 0x42424242;
			some_pod->b = 0xB16B00B5;
			some_pod->c = 0xD0D0DADAD0D0DADA;
			W_DELETE(some_pod, arena);
		}
		else
		{
			NonPOD* some_non_pod = W_NEW(NonPOD, arena)(10,8);
			W_DELETE(some_non_pod, arena);
		}
		if(ii==5)
		{
			POD* pod_array = W_NEW_ARRAY_ALIGN(POD[10], arena, 32);
			for(int ii=0; ii<10; ++ii)
			{
				pod_array[ii].a = 0x42424242;
				pod_array[ii].b = 0xB16B00B5;
				pod_array[ii].c = 0xD0D0DADAD0D0DADA;
			}
			W_DELETE_ARRAY(pod_array, arena);
		}
	}
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(area.begin()), 1_kB);

	SUCCEED();
}
