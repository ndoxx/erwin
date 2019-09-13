// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>
#include "erwin.h"

#include "render/buffer.h"
#include "render/shader.h"

#include "platform/ogl_shader.h"
#include "platform/ogl_texture.h"


using namespace erwin;

static std::unique_ptr<Renderer2D> s_renderer_2D;

class Layer2D: public Layer
{
public:
	Layer2D(const std::string& name):
	Layer("Layer2D_" + name)
	{

	}

	~Layer2D()
	{ 

	}

	virtual bool on_event(const MouseButtonEvent& event) override
	{
		DLOGN("event") << get_name() << " -> Handled event: " << event << std::endl;
		return true;
	}

	virtual void on_imgui_render() override
	{
	    ImGui::Begin("Renderer2D");

        if(ImGui::Checkbox("Profile", &enable_profiling_))
        	s_renderer_2D->set_profiling_enabled(enable_profiling_);

	    ImGui::End();
	}

	virtual void on_attach() override
	{
		s_renderer_2D = std::make_unique<Renderer2D>();

		// Colored triangle
		BufferLayout vertex_color_layout =
		{
		    {"a_position"_h, ShaderDataType::Vec3},
		    {"a_color"_h,    ShaderDataType::Vec3},
		};
		float tri_vdata[18] = 
		{
			-0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
			 0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f
		};
		uint32_t tri_idata[3] =
		{
			0, 1, 2
		};

		tri_vb_ = std::shared_ptr<VertexBuffer>(VertexBuffer::create(tri_vdata, 18, vertex_color_layout));
		tri_ib_ = std::shared_ptr<IndexBuffer>(IndexBuffer::create(tri_idata, 3, DrawPrimitive::Triangles));
		tri_va_ = std::shared_ptr<VertexArray>(VertexArray::create());
		tri_va_->set_index_buffer(tri_ib_);
		tri_va_->set_vertex_buffer(tri_vb_);

		// Textured square
		BufferLayout vertex_tex_layout =
		{
		    {"a_position"_h, ShaderDataType::Vec3},
		    {"a_uv"_h,       ShaderDataType::Vec2},
		};
		float sq_vdata[20] = 
		{
			-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,   1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f,   1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f,   0.0f, 1.0f
		};
		uint32_t sq_idata[6] =
		{
			0, 1, 2,   2, 3, 0
		};

		sq_vb_ = std::shared_ptr<VertexBuffer>(VertexBuffer::create(sq_vdata, 20, vertex_tex_layout));
		sq_ib_ = std::shared_ptr<IndexBuffer>(IndexBuffer::create(sq_idata, 6, DrawPrimitive::Triangles));
		sq_va_ = std::shared_ptr<VertexArray>(VertexArray::create());
		sq_va_->set_index_buffer(sq_ib_);
		sq_va_->set_vertex_buffer(sq_vb_);

		dirt_tex_ = Texture2D::create("textures/dirt01_albedo.png");

		// Load shaders
		Renderer2D::shader_bank.load("shaders/color_shader.glsl");
		Renderer2D::shader_bank.load("shaders/tex_shader.glsl");
	}

protected:
	virtual void on_update() override
	{
		s_renderer_2D->begin_scene(get_priority());
		{
			// TODO: group shader hname & ShaderParameters in Material class
			// TODO: handle transforms
			// TODO: group material & transform in Mesh class

			// Per-batch render state mutations
			RenderState render_state;
			render_state.render_target = RenderTarget::Default;
			render_state.rasterizer_state = CullMode::Back;
			render_state.blend_state = BlendState::Opaque;
			s_renderer_2D->submit(render_state);

			// Per-instance draw commands
			ShaderParameters sq_params;
			sq_params.set_texture_slot("us_albedo"_h, dirt_tex_);
			s_renderer_2D->submit(sq_va_, "tex_shader"_h, sq_params);

			ShaderParameters tri_params;
			s_renderer_2D->submit(tri_va_, "color_shader"_h, tri_params);
		}
		s_renderer_2D->end_scene();
	}

private:
	std::shared_ptr<VertexBuffer> tri_vb_;
	std::shared_ptr<IndexBuffer>  tri_ib_;
	std::shared_ptr<VertexArray>  tri_va_;

	std::shared_ptr<VertexBuffer> sq_vb_;
	std::shared_ptr<IndexBuffer>  sq_ib_;
	std::shared_ptr<VertexArray>  sq_va_;

	std::shared_ptr<Texture2D> dirt_tex_;

	bool enable_profiling_ = false;
};

class Sandbox: public Application
{
public:
	Sandbox()
	{
		EVENTBUS.subscribe(this, &Sandbox::on_key_pressed_event);

		filesystem::set_asset_dir("source/Applications/Sandbox/assets");
		push_layer(new Layer2D("A"));
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
