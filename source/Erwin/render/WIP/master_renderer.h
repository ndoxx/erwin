#pragma once

#include <cstdint>
#include "render/WIP/command_queue.h"

namespace erwin
{
namespace WIP
{

class MasterRenderer
{
public:
	friend class CommandQueue;

	static void init();
	static void shutdown();

	static CommandQueue& get_queue(int name);
	static void flush();

	static void test_submit(RenderCommand* cmd);
	static void test();

private:
	struct dispatch
	{
		static void create_index_buffer(RenderCommand* cmd);
		static void create_vertex_buffer_layout(RenderCommand* cmd);
		static void create_vertex_buffer(RenderCommand* cmd);
		static void create_vertex_array(RenderCommand* cmd);
		static void create_uniform_buffer(RenderCommand* cmd);
	};
};

} // namespace WIP
} // namespace erwin