#pragma once

#include "erwin.h"

#include "render/render_device.h"

// #include "render/buffer.h"
// #include "render/shader.h"
// #include "platform/ogl_shader.h"
// #include "platform/ogl_texture.h"

using namespace erwin;

class LayerBatch2D: public Layer
{
public:
	LayerBatch2D();
	~LayerBatch2D() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;

protected:
	virtual void on_update(GameClock& clock) override;

	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;

private:
	WScope<Renderer2D> renderer_2D_;
	TextureAtlas atlas_;
	OrthographicCamera2DController camera_ctl_;

	std::vector<hash_t> tiles_;

	RenderStats render_stats_;
	bool enable_profiling_ = false;
	uint32_t frame_cnt_ = 0;
	float fps_ = 60.f;
	float tt_ = 0.f;

	int len_grid_ = 100;
	int batch_size_ = 8192;
	bool trippy_mode_ = false;
};