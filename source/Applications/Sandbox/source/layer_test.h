#pragma once

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

// #include "render/buffer.h"
// #include "render/shader.h"
// #include "platform/ogl_shader.h"
// #include "platform/ogl_texture.h"
#include "render/WIP/master_renderer.h"
#include "memory/memory_utils.h"

using namespace erwin;
using namespace erwin::WIP;

class LayerTest: public Layer
{
public:
	LayerTest(): Layer("TestLayer")
	{
	    memory::hex_dump_highlight(0xf0f0f0f0, WCB(100,0,0));
	    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,0,100));
	    memory::hex_dump_highlight(0xd0d0d0d0, WCB(200,100,0));
	}

	~LayerTest() = default;

	virtual void on_imgui_render() override
	{

	}

	virtual void on_attach() override
	{
		MasterRenderer::init();

		IndexBufferHandle ibo_handle;
		VertexBufferLayoutHandle vbl_handle;
		VertexBufferHandle vbo_handle;
		VertexArrayHandle va_handle;
		uint32_t index_data[6] = { 0, 1, 2, 2, 3, 1 };
		float sq_vdata[20] = 
		{
			-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f,   0.0f, 1.0f
		};

		auto& rq = MasterRenderer::get_queue(CommandQueue::Resource);
		ibo_handle = rq.create_index_buffer(100, index_data, 6, DrawPrimitive::Triangles);
		auto ibo_handle2 = rq.create_index_buffer(99, index_data, 6, DrawPrimitive::Triangles);
		rq.destroy_index_buffer(98, ibo_handle2);
		vbl_handle = rq.create_vertex_buffer_layout(90,
									 			    {
				    				 			    	{"a_position"_h, ShaderDataType::Vec3},
    								 			    	{"a_uv"_h,       ShaderDataType::Vec2},
    								 			    });
		vbo_handle = rq.create_vertex_buffer(80, vbl_handle, sq_vdata, 20, DrawMode::Static);
		va_handle = rq.create_vertex_array(70, vbo_handle, ibo_handle);

		auto ubo_handle = rq.create_uniform_buffer(60, "layout_xyz", nullptr, 64, DrawMode::Dynamic);
		auto ssbo_handle = rq.create_shader_storage_buffer(50, "layout_xyz", nullptr, 10*64, DrawMode::Dynamic);

		rq.destroy_vertex_array(45, va_handle);
		rq.destroy_vertex_buffer(40, vbo_handle);
		rq.destroy_index_buffer(35, ibo_handle);
		rq.destroy_vertex_buffer_layout(30, vbl_handle);

		MasterRenderer::test();
		MasterRenderer::flush();

		uint8_t ubo_data[64];
		uint8_t ssbo_data[10*64];
		rq.update_uniform_buffer(100, ubo_handle, ubo_data, 64);
		rq.update_shader_storage_buffer(90, ssbo_handle, ssbo_data, 10*64);

		rq.destroy_uniform_buffer(20, ubo_handle);
		rq.destroy_shader_storage_buffer(10, ssbo_handle);
		MasterRenderer::flush();
	}

	virtual void on_detach() override
	{
		MasterRenderer::shutdown();
	}


protected:
	virtual void on_update(GameClock& clock) override
	{

	}

	virtual bool on_event(const MouseButtonEvent& event) override
	{
		return false;
	}

	virtual bool on_event(const WindowResizeEvent& event) override
	{
		return false;
	}

	virtual bool on_event(const MouseScrollEvent& event) override
	{
		return false;
	}


private:

};