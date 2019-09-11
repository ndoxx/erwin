#include "glad/glad.h"
#include <memory>
#include <iostream>
#include "erwin.h"

#include "render/buffer.h"
#include "render/shader.h"
#include "render/render_device.h"

using namespace erwin;

class TestLayer: public Layer
{
public:
	TestLayer(const std::string& name):
	Layer("TestLayer_" + name)
	{

	}

	~TestLayer()
	{ 

	}

	virtual bool on_event(const MouseButtonEvent& event) override
	{
		DLOGN("event") << get_name() << " -> Handled event: " << event << std::endl;
		return true;
	}

	virtual void on_imgui_render() override
	{
	    ImGui::Begin("Test Widget");

	    ImGui::End();
	}

	virtual void on_attach() override
	{
		BufferLayout vertex_color_layout =
		{
		    {"a_position"_h, ShaderDataType::Vec3},
		    {"a_color"_h,    ShaderDataType::Vec3},
		};

		float vdata[18] = 
		{
			-0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
			 0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f
		};
		vb_ = std::shared_ptr<VertexBuffer>(VertexBuffer::create(vdata, 18, vertex_color_layout));
	
		uint32_t idata[3] =
		{
			0, 1, 2
		};
		ib_ = std::shared_ptr<IndexBuffer>(IndexBuffer::create(idata, 3));

		va_ = std::shared_ptr<VertexArray>(VertexArray::create());
		va_->set_index_buffer(ib_);
		va_->add_vertex_buffer(vb_);

    	std::string shader_source = R"(
    		#type vertex
			#version 410 core

			layout(location = 0) in vec3 in_position;
			layout(location = 1) in vec3 in_color;

			out vec3 v_color;

			void main()
			{
    			gl_Position = vec4(in_position, 1.f);
    			v_color = in_color;
			}

			#type fragment
			#version 410 core

			in vec3 v_color;
			layout(location = 0) out vec4 out_color;

			void main()
			{
				out_color = vec4(v_color,1.0f);
			}
    	)";

    	shader_ = Shader::create("test_shader", shader_source);

		Gfx::device->bind_default_frame_buffer();
		Gfx::device->set_cull_mode(CullMode::None);
	}

	virtual void on_detach() override
	{
		DLOG("application",1) << "plop" << std::endl;
	}

protected:
	virtual void on_update() override
	{
    	shader_->bind();
    	Gfx::device->draw_indexed(va_);
	}

private:
	std::shared_ptr<VertexBuffer> vb_;
	std::shared_ptr<IndexBuffer> ib_;
	std::shared_ptr<VertexArray> va_;
	std::shared_ptr<Shader> shader_;
};

class Sandbox: public Application
{
public:
	Sandbox()
	{
		EVENTBUS.subscribe(this, &Sandbox::on_key_pressed_event);

		push_layer(new TestLayer("A"));
	}

	~Sandbox()
	{

	}

	bool on_key_pressed_event(const KeyboardEvent& e)
	{
		// Terminate on ESCAPE
		if(e.pressed && e.key == keymap::WKEY::ESCAPE)
			EVENTBUS.publish(WindowCloseEvent());

		return false;
	}
};

Application* erwin::create_application()
{
	return new Sandbox();
}
