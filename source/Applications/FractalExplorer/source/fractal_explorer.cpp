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

#include "glm/glm.hpp"

using namespace erwin;

struct MandelbrotData
{
	glm::mat4 view_projection;
    glm::vec4 palette = glm::vec4(0.3f,0.7f,0.9f,0.f);
    float max_iter = 20.f;
    float escape_radius = 2.f;
    float time = 0.f;
    float attenuation = 1.f;
};

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
	        ImGui::SliderFloat("Escape radius", &data_.escape_radius, 2.f, 100.f);
	        ImGui::SliderFloat("Attenuation", &data_.attenuation, 0.f, 5.f);
	        ImGui::SliderFloat3("Palette", (float*)&data_.palette, 0.0f, 1.0f);
	        ImGui::SliderFloat("Animation speed", &speed_, 0.f, 1.f);
	    }
	}

	virtual void on_attach() override
	{
		shader_bank_.load(filesystem::get_asset_dir() / "shaders/mandelbrot.glsl");
		// shader_bank_.load(filesystem::get_asset_dir() / "shaders/mandelbrot.spv");

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

		mandel_ubo_ = UniformBuffer::create("mandelbrot_layout", nullptr, sizeof(MandelbrotData), DrawMode::Dynamic);
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


		data_.max_iter = (float)max_iter_;
		data_.time = float(2*M_PI*tt_);
		data_.view_projection = glm::inverse(camera_ctl_.get_camera().get_view_projection_matrix());
		mandel_ubo_->map(&data_);

		const auto& shader = shader_bank_.get("mandelbrot"_h);
		shader.bind();
		shader.attach_uniform_buffer(*mandel_ubo_);
		// static_cast<const OGLShader&>(shader).send_uniform("u_view_projection"_h, transform);

   		Gfx::device->draw_indexed(*quad_va_);
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
	WRef<UniformBuffer> mandel_ubo_;
	float tt_ = 0.f;
	float fps_;
	bool show_menu_;

	MandelbrotData data_;
	float speed_ = 0.1f;
	int max_iter_ = 100;
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
