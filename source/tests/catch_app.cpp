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
#include "entity/entity_manager.h"

using namespace erwin;

int main(int argc, char* argv[])
{
    WLOGGER(create_channel("thread", 0));
    WLOGGER(create_channel("event", 0));
	WLOGGER(create_channel("rng", 0));
    WLOGGER(create_channel("memory", 0));
	WLOGGER(create_channel("entity", 0));
    WLOGGER(attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>()));
    WLOGGER(set_single_threaded(true));

    memory::hex_dump_highlight(0xf0f0f0f0, WCB(200,50,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,50,200));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(50,120,0));

	int result = Catch::Session().run( argc, argv );

	// global clean-up...

	return result;
}