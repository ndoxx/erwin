// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>

#define W_ENTRY_POINT
#include "erwin.h"
#include "layer_2d.h"
#include "layer_3d.h"
#include "layer_3d_deferred.h"
#include "layer_presentation.h"
#include "layer_debug.h"

using namespace erwin;

class Sandbox: public Application
{
public:
	Sandbox() = default;
	~Sandbox() = default;

	virtual void on_client_init() override;
	virtual void on_load() override;
	virtual void on_imgui_render() override;
	bool on_keyboard_event(const KeyboardEvent& e);

	void toggle_layer_3d();
	void toggle_layer_3d_deferred();
	void toggle_layer_2d();

	void window_layer_config(bool* p_open);
	void window_post_processing(bool* p_open);
	void window_lighting(bool* p_open);
#ifdef W_PROFILE
	void window_profiling(bool* p_open);
	void window_render_stats(bool* p_open);
#endif

private:
	bool layer3d_enabled_ = true;
	bool layer3d_deferred_enabled_ = false;
	bool layer2d_enabled_ = false;

#ifdef W_PROFILE
	bool enable_runtime_profiling_ = false;
	bool frame_profiling_ = false;
	int profile_num_frames_ = 60;
	int frames_counter_ = 0;
#endif

	Layer3D* layer_3d_;
	Layer3DDeferred* layer_3d_deferred_;
	Layer2D* layer_2d_;
	PresentationLayer* presentation_layer_;
	DebugLayer* debug_layer_;
};

Application* erwin::create_application()
{
	return new Sandbox();
}
