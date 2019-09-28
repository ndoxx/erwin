// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>

#define W_ENTRY_POINT
#include "erwin.h"

#include "render/buffer.h"
#include "render/shader.h"
#include "render/render_device.h"
#include "platform/ogl_shader.h"
#include "platform/ogl_texture.h"


using namespace erwin;

class FractalLayer: public Layer
{
public:
	FractalLayer():
	Layer("FractalLayer"),
	camera_ctl_(1280.f/1024.f, 1.f),
	show_menu_(true)
	{

	}

	~FractalLayer() = default;

	virtual bool on_event(const MouseButtonEvent& event) override
	{
		DLOGN("event") << get_name() << " -> Handled event: " << event << std::endl;
		return true;
	}

	virtual void on_imgui_render() override
	{
		if(show_menu_)
		{
		    ImGui::Begin("Mandelbrot explorer");
	        ImGui::SliderInt("Max iterations", &max_iter_, 1, 200);
	        ImGui::SliderFloat("Escape radius", &escape_radius_, 2.f, 100.f);
	        ImGui::SliderFloat("Attenuation", &attenuation_, 0.f, 5.f);
	        ImGui::SliderFloat("Animation speed", &speed_, 0.f, 1.f);
	        ImGui::SliderFloat3("Palette", (float*)&palette_, 0.0f, 1.0f);
	    }
	}

	virtual void on_attach() override
	{
		shader_bank_.load("shaders/mandelbrot.glsl");

		// Create vertex array with a quad
		BufferLayout vertex_tex_layout =
		{
		    {"a_position"_h, ShaderDataType::Vec3},
		    {"a_uv"_h,       ShaderDataType::Vec2},
		};
		float sq_vdata[20] = 
		{
			-1.0f, -1.0f, 0.0f,   -1.0f, -1.0f,
			 1.0f, -1.0f, 0.0f,   1.0f, -1.0f,
			 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f,   -1.0f, 1.0f
		};
		uint32_t sq_idata[6] =
		{
			0, 1, 2,   2, 3, 0
		};
		auto quad_vb = VertexBuffer::create(sq_vdata, 20, vertex_tex_layout);
		auto quad_ib = IndexBuffer::create(sq_idata, 6, DrawPrimitive::Triangles);
		quad_va_ = VertexArray::create();
		quad_va_->set_index_buffer(quad_ib);
		quad_va_->set_vertex_buffer(quad_vb);
	}

protected:
	virtual void on_update(GameClock& clock) override
	{
		// Update
		camera_ctl_.update(clock);

		// Render submission
		float dt = clock.get_frame_duration();
		fps_ = 1.f/dt;

		tt_ += speed_*dt;
		if(tt_>=1.f)
			tt_ = 0.f;

		const auto& camera = camera_ctl_.get_camera();
		glm::mat4 transform = glm::inverse(camera.get_view_projection_matrix());

		const auto& shader = shader_bank_.get("mandelbrot"_h);
		shader.bind();

		static_cast<const OGLShader&>(shader).send_uniform("u_max_iter"_h, (float)max_iter_);
		static_cast<const OGLShader&>(shader).send_uniform("u_view_projection"_h, transform);
		static_cast<const OGLShader&>(shader).send_uniform("u_palette"_h, palette_);
		static_cast<const OGLShader&>(shader).send_uniform("u_escape_radius"_h, escape_radius_);
		static_cast<const OGLShader&>(shader).send_uniform("u_time"_h, float(2*M_PI*tt_));
		static_cast<const OGLShader&>(shader).send_uniform("u_atten"_h, attenuation_);

   		Gfx::device->draw_indexed(quad_va_);
		shader.unbind();
	}

	virtual bool on_event(const WindowResizeEvent& event) override
	{
		camera_ctl_.on_window_resize_event(event);
		return false;
	}

	virtual bool on_event(const MouseScrollEvent& event) override
	{
		camera_ctl_.on_mouse_scroll_event(event);
		return false;
	}

	virtual bool on_event(const KeyboardEvent& event) override
	{
		if(event.pressed && event.key == keymap::WKEY::TAB)
			show_menu_ = !show_menu_;
		return false;
	}

private:
	OrthographicCamera2DController camera_ctl_;
	ShaderBank shader_bank_;
	WRef<VertexArray> quad_va_;
	float tt_ = 0.f;
	float fps_;
	bool show_menu_;

	int max_iter_ = 100;
	float escape_radius_ = 75.9f;
	float attenuation_ = 0.943f;
	float speed_ = 0.1f;
	glm::vec3 palette_ = glm::vec3(0.297f,0.938f,1.0f);
};

class FractalExplorer: public Application
{
public:
	FractalExplorer()
	{
		EVENTBUS.subscribe(this, &FractalExplorer::on_keyboard_event);

		filesystem::set_asset_dir("source/Applications/FractalExplorer/assets");
		push_layer(new FractalLayer());
	}

	~FractalExplorer()
	{

	}

	bool on_keyboard_event(const KeyboardEvent& e)
	{
		// Terminate on ESCAPE
		if(e.pressed && e.key == keymap::WKEY::ESCAPE)
			EVENTBUS.publish(WindowCloseEvent());

		return false;
	}
};

Application* erwin::create_application()
{
	return new FractalExplorer();
}
