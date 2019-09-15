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
        	batch_renderer_2D_->set_profiling_enabled(enable_profiling_);

	    ImGui::End();
	}

	virtual void on_attach() override
	{
		batch_renderer_2D_ = std::make_unique<BatchRenderer2D>(8192);

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
		BatchRenderer2D::shader_bank.load("shaders/color_shader.glsl");
		BatchRenderer2D::shader_bank.load("shaders/tex_shader.glsl");
	}

protected:
	virtual void on_update(GameClock& clock) override
	{
		batch_renderer_2D_->begin_scene(get_priority());
		{
			// TODO: group shader hname & ShaderParameters in Material class
			// TODO: handle transforms
			// TODO: group material & transform in Mesh class

			// Per-batch render state mutations
			RenderState render_state;
			render_state.render_target = RenderTarget::Default;
			render_state.rasterizer_state = CullMode::Back;
			render_state.blend_state = BlendState::Opaque;
			batch_renderer_2D_->submit(render_state);

			// Per-instance draw commands
			ShaderParameters sq_params;
			sq_params.set_texture_slot("us_albedo"_h, dirt_tex_);
			batch_renderer_2D_->submit(sq_va_, "tex_shader"_h, sq_params);

			ShaderParameters tri_params;
			batch_renderer_2D_->submit(tri_va_, "color_shader"_h, tri_params);
		}
		batch_renderer_2D_->end_scene();
	}

private:
	std::unique_ptr<BatchRenderer2D> batch_renderer_2D_;

	std::shared_ptr<VertexBuffer> tri_vb_;
	std::shared_ptr<IndexBuffer>  tri_ib_;
	std::shared_ptr<VertexArray>  tri_va_;

	std::shared_ptr<VertexBuffer> sq_vb_;
	std::shared_ptr<IndexBuffer>  sq_ib_;
	std::shared_ptr<VertexArray>  sq_va_;

	std::shared_ptr<Texture2D> dirt_tex_;

	bool enable_profiling_ = false;
};


class LayerBatch2D: public Layer
{
public:
	LayerBatch2D(const std::string& name):
	Layer("LayerBatch2D_" + name)
	{

	}

	~LayerBatch2D() = default;

	virtual bool on_event(const MouseButtonEvent& event) override
	{
		DLOGN("event") << get_name() << " -> Handled event: " << event << std::endl;
		return true;
	}

	virtual void on_imgui_render() override
	{
		static const char* rd_items[] = {"Batch renderer", "Instanced renderer"};
	    
	    ImGui::Begin("BatchRenderer2D");

        	if(ImGui::Checkbox("Profile", &enable_profiling_))
        		renderer_2D_->set_profiling_enabled(enable_profiling_);

            ImGui::Separator();

            static int s_renderer_impl = 0;
            ImGui::WCombo("##renderersel", "Renderer", s_renderer_impl, 2, rd_items);
            if(ImGui::Button("Swap implementation"))
            {
            	switch(s_renderer_impl)
            	{
            		case 0: renderer_2D_ = std::make_unique<BatchRenderer2D>(batch_size_); break;
            		case 1: renderer_2D_ = std::make_unique<InstanceRenderer2D>(batch_size_); break;
            		default: break;
            	}
            	renderer_2D_->set_profiling_enabled(enable_profiling_);
            }
            ImGui::Separator();

            ImGui::SliderInt("Grid size", &len_grid_, 10, 500);
            if(ImGui::SliderInt("Batch size", &batch_size_, 200, 10000))
            {
				renderer_2D_->set_batch_size(batch_size_);
            }
        	ImGui::Text("Drawing %d squares.", len_grid_*len_grid_);
        	ImGui::Checkbox("Trippy mode", &trippy_mode_);

	    ImGui::End();

	    if(enable_profiling_)
	    {
	    	ImGui::Begin("Stats");
            	ImGui::Text("#Batches: %d", render_stats_.batches);
            	ImGui::Separator();
            	ImGui::PlotVar("Draw time (µs)", render_stats_.render_time, 0.0f, 7000.f);
            	ImGui::PlotVar("FPS (Hz)", fps_, 0.0f, 100.f);
	    	
	            if(++frame_cnt_>200)
	            {
	                frame_cnt_ = 0;
	                ImGui::PlotVarFlushOldEntries();
	            }
	    	ImGui::End();
	    }
	}

	virtual void on_attach() override
	{
		renderer_2D_ = std::make_unique<BatchRenderer2D>(batch_size_);
	}

protected:
	virtual void on_update(GameClock& clock) override
	{
		float dt = clock.get_frame_duration();
		fps_ = 1.f/dt;

		tt_ += dt;
		if(tt_>=5.f)
			tt_ = 0.f;

		renderer_2D_->begin_scene(get_priority());
		{
			RenderState render_state;
			render_state.render_target = RenderTarget::Default;
			render_state.rasterizer_state = CullMode::None;
			render_state.blend_state = BlendState::Opaque;
			renderer_2D_->submit(render_state);

			// Draw a grid of quads
			for(int xx=0; xx<len_grid_; ++xx)
			{
				float xx_offset = trippy_mode_ ? 3.0f/len_grid_ * cos(2*2*M_PI*xx/(1.f+len_grid_))*sin(0.2f*2*M_PI*tt_) : 0.f;

				for(int yy=0; yy<len_grid_; ++yy)
				{
					float yy_offset = trippy_mode_ ? 3.0f/len_grid_ * sin(2*2*M_PI*yy/(1.f+len_grid_))*cos(0.2f*2*M_PI*tt_) : 0.f;

					renderer_2D_->draw_quad(
					{-0.95f + 1.9f*xx/float(len_grid_-1) + xx_offset, -0.95f + 1.9f*yy/float(len_grid_-1) + yy_offset},
					{1.0f/len_grid_,1.0f/len_grid_},
					{xx/float(len_grid_-1),yy/float(len_grid_-1),1.f-xx/float(len_grid_-1)});
				}
			}
		}
		renderer_2D_->end_scene();

		render_stats_ = renderer_2D_->get_stats();
	}

private:
	std::unique_ptr<Renderer2D> renderer_2D_;
	RenderStats render_stats_;
	bool enable_profiling_ = false;
	uint32_t frame_cnt_ = 0;
	float fps_ = 60.f;
	float tt_ = 0.f;

	int len_grid_ = 100;
	int batch_size_ = 8192;
	bool trippy_mode_ = false;
};

class Sandbox: public Application
{
public:
	Sandbox()
	{
		EVENTBUS.subscribe(this, &Sandbox::on_key_pressed_event);

		filesystem::set_asset_dir("source/Applications/Sandbox/assets");
		push_layer(new LayerBatch2D("A"));
		// push_layer(new Layer2D("A"));
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
