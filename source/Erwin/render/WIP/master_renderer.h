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

	static void test();

	struct dispatch
	{
		static void create_index_buffer(memory::LinearBuffer<>& buf);
		static void create_vertex_buffer_layout(memory::LinearBuffer<>& buf);
		static void create_vertex_buffer(memory::LinearBuffer<>& buf);
		static void create_vertex_array(memory::LinearBuffer<>& buf);
		static void create_uniform_buffer(memory::LinearBuffer<>& buf);
		static void create_shader_storage_buffer(memory::LinearBuffer<>& buf);

		static void update_index_buffer(memory::LinearBuffer<>& buf);
		static void update_vertex_buffer(memory::LinearBuffer<>& buf);
		static void update_uniform_buffer(memory::LinearBuffer<>& buf);
		static void update_shader_storage_buffer(memory::LinearBuffer<>& buf);

		static void destroy_index_buffer(memory::LinearBuffer<>& buf);
		static void destroy_vertex_buffer_layout(memory::LinearBuffer<>& buf);
		static void destroy_vertex_buffer(memory::LinearBuffer<>& buf);
		static void destroy_vertex_array(memory::LinearBuffer<>& buf);
		static void destroy_uniform_buffer(memory::LinearBuffer<>& buf);
		static void destroy_shader_storage_buffer(memory::LinearBuffer<>& buf);
	};
};

} // namespace WIP
} // namespace erwin