// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>

#define W_ENTRY_POINT
#include "erwin.h"
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
		shader_ = AssetManager::load_shader("shaders/mandelbrot.glsl");
		mandel_ubo_ = Renderer::create_uniform_buffer("mandelbrot_layout", nullptr, sizeof(MandelbrotData), UsagePattern::Dynamic);
		Renderer::shader_attach_uniform_buffer(shader_, mandel_ubo_);
	}

	virtual void on_detach() override
	{
		Renderer::destroy(mandel_ubo_);
		AssetManager::release(shader_);
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

		RenderState pass_state;
		pass_state.render_target = Renderer::default_render_target();
		pass_state.rasterizer_state.cull_mode = CullMode::Back;
		pass_state.blend_state = BlendState::Opaque;
		pass_state.depth_stencil_state.depth_test_enabled = false;
		pass_state.rasterizer_state.clear_color = glm::vec4(0.2f,0.2f,0.2f,0.f);

		DrawCall dc(DrawCall::Indexed, pass_state.encode(), shader_, CommonGeometry::get_vertex_array("quad"_h));
		dc.add_dependency(Renderer::update_uniform_buffer(mandel_ubo_, &data_, sizeof(MandelbrotData), DataOwnership::Copy));
		Renderer::submit("Presentation"_h, dc);
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
	ShaderHandle shader_;
	UniformBufferHandle mandel_ubo_;
	OrthographicCamera2DController camera_ctl_;

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
	FractalExplorer() = default;
	~FractalExplorer() = default;

	virtual void on_client_init() override
	{
		filesystem::set_asset_dir("source/Applications/FractalExplorer/assets");
		filesystem::set_client_config_dir("source/Applications/FractalExplorer/config");
		this->add_configuration("client.xml");
	}

	virtual void on_load() override
	{
		EVENTBUS.subscribe(this, &FractalExplorer::on_keyboard_event);

		push_layer(new FractalLayer());
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
