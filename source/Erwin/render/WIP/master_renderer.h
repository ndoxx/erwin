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
	friend struct RenderCommand;

	static void init();
	static void shutdown();

	static void test_submit(RenderCommand* cmd);

private:
	struct dispatch
	{
		static void create_index_buffer(RenderCommand* cmd);
	};
};

} // namespace WIP
} // namespace erwin