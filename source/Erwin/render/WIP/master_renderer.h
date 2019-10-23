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
	};
};

namespace hnd
{
	template<typename HandleT>
	extern HandleT* get() { return nullptr; }
	template<typename HandleT>
	extern void release(HandleT*) { }

	template<> [[maybe_unused]] IndexBufferHandle*         get();
	template<> [[maybe_unused]] VertexBufferLayoutHandle*  get();
	template<> [[maybe_unused]] VertexBufferHandle*        get();
	template<> [[maybe_unused]] VertexArrayHandle*         get();
	template<> [[maybe_unused]] UniformBufferHandle*       get();
	template<> [[maybe_unused]] ShaderStorageBufferHandle* get();
	template<> [[maybe_unused]] TextureHandle*             get();
	template<> [[maybe_unused]] ShaderHandle*              get();

	template<> [[maybe_unused]] void release(IndexBufferHandle* handle);
	template<> [[maybe_unused]] void release(VertexBufferLayoutHandle* handle);
	template<> [[maybe_unused]] void release(VertexBufferHandle* handle);
	template<> [[maybe_unused]] void release(VertexArrayHandle* handle);
	template<> [[maybe_unused]] void release(UniformBufferHandle* handle);
	template<> [[maybe_unused]] void release(ShaderStorageBufferHandle* handle);
	template<> [[maybe_unused]] void release(TextureHandle* handle);
	template<> [[maybe_unused]] void release(ShaderHandle* handle);
} // namespace hnd

} // namespace WIP
} // namespace erwin