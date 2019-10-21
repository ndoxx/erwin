/*
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
*/

#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"

#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"
#include "memory/memory_utils.h"

using namespace erwin;

int main(int argc, char* argv[])
{
    WLOGGER.create_channel("memory", 3);
    WLOGGER.attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.set_single_threaded(true);

    memory::hex_dump_highlight(0xf0f0f0f0, WCB(100,0,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,0,100));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(200,100,0));

	int result = Catch::Session().run( argc, argv );

	// global clean-up...

	return result;
}