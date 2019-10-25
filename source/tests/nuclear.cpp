#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>

#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"

// #include "render/render_queue.hpp"
#include "render/WIP/command_queue.h"

#include "memory/handle_pool.h"
#include "memory/memory_utils.h"
#include "memory/memory.hpp"

using namespace erwin;
using namespace WIP;



int main(int argc, char** argv)
{
	memory::HeapArea area(10_kB);
	memory::LinearBuffer<> lbuf(area.require_block(1_kB));

	auto* start = lbuf.get_head();
	{
		uint32_t size = 0x42424242;
		uint64_t flags = 0x8282828282828282;
		std::string name("plop plip plop");
		lbuf.write(&size);
		lbuf.write(&flags);
		lbuf.write_str(name);
	}
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(area.begin()), 128_B);
	{
		uint32_t size;
		uint64_t flags;
		std::string name;
		lbuf.seek(start);
		lbuf.read(&size);
		lbuf.read(&flags);
		lbuf.read_str(name);

		std::cout << name << " " << std::hex << size << " " << flags << std::endl;
	}

	start = lbuf.get_head();
	{
		uint32_t size = 0x69696969;
		uint64_t flags = 0xB16B00B5B16B00B5;
		std::string name("roubignoles");
		lbuf.write(&size);
		lbuf.write(&flags);
		lbuf.write_str(name);
	}
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(area.begin()), 128_B);
	{
		uint32_t size;
		uint64_t flags;
		std::string name;
		lbuf.seek(start);
		lbuf.read(&size);
		lbuf.read(&flags);
		lbuf.read_str(name);

		std::cout << name << " " << std::hex << size << " " << flags << std::endl;
	}

	return 0;
}
