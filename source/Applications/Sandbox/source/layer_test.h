#pragma once

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

// #include "render/buffer.h"
// #include "render/shader.h"
// #include "platform/ogl_shader.h"
// #include "platform/ogl_texture.h"
#include "render/WIP/main_renderer.h"
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
		MainRenderer::init();

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

		auto& rq = MainRenderer::get_queue(MainRenderer::Resource);
		// auto shader_handle = rq.create_shader(filesystem::get_system_asset_dir() / "shaders/color_inst_shader.spv", "color_inst_shader");
		ibo_handle = rq.create_index_buffer(index_data, 6, DrawPrimitive::Triangles);
		auto ibo_handle2 = rq.create_index_buffer(index_data, 6, DrawPrimitive::Triangles);
		rq.destroy_index_buffer(ibo_handle2);
		vbl_handle = rq.create_vertex_buffer_layout({
				    				 			    	{"a_position"_h, ShaderDataType::Vec3},
    								 			    	{"a_uv"_h,       ShaderDataType::Vec2},
    								 			    });
		vbo_handle = rq.create_vertex_buffer(vbl_handle, sq_vdata, 20, DrawMode::Static);
		va_handle = rq.create_vertex_array(vbo_handle, ibo_handle);

		auto ubo_handle = rq.create_uniform_buffer("layout_xyz", nullptr, 64, DrawMode::Dynamic);
		auto ssbo_handle = rq.create_shader_storage_buffer("layout_xyz", nullptr, 10*64, DrawMode::Dynamic);

		rq.destroy_vertex_array(va_handle);
		rq.destroy_vertex_buffer(vbo_handle);
		rq.destroy_index_buffer(ibo_handle);
		rq.destroy_vertex_buffer_layout(vbl_handle);

		MainRenderer::DEBUG_test();
		MainRenderer::flush();

		uint8_t ubo_data[64];
		uint8_t ssbo_data[10*64];
		rq.update_uniform_buffer(ubo_handle, ubo_data, 64);
		rq.update_shader_storage_buffer(ssbo_handle, ssbo_data, 10*64);

		rq.destroy_uniform_buffer(ubo_handle);
		rq.destroy_shader_storage_buffer(ssbo_handle);
		// rq.destroy_shader(shader_handle);
		MainRenderer::flush();
	}

	virtual void on_detach() override
	{
		MainRenderer::shutdown();
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