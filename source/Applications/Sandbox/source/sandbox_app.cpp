// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>

#define W_ENTRY_POINT
#include "erwin.h"
#include "layer_2d.h"
#include "layer_3d.h"
#include "layer_presentation.h"
#include "layer_debug.h"

using namespace erwin;


class Sandbox: public Application
{
public:
	Sandbox() = default;
	~Sandbox() = default;

	virtual void on_client_init() override
	{
		filesystem::set_asset_dir("source/Applications/Sandbox/assets");
		filesystem::set_client_config_dir("source/Applications/Sandbox/config");
		this->add_configuration("sandbox.xml");
	}

	virtual void on_load() override
	{
		EVENTBUS.subscribe(this, &Sandbox::on_keyboard_event);

		push_layer(new Layer3D());
		push_layer(new Layer2D());
		presentation_layer_ = new PresentationLayer();
		push_overlay(presentation_layer_);
		push_overlay(new DebugLayer());

	    set_layer_enabled(0, layer3d_enabled_);
	    set_layer_enabled(1, layer2d_enabled_);
	}

	virtual void on_imgui_render() override
	{
	    ImGui::Begin("Layers");
	    	if(ImGui::Checkbox("3D Forward", &layer3d_enabled_))
	    	{
	    		set_layer_enabled(0, layer3d_enabled_);
	    		presentation_layer_->enable_forward_rendering(layer3d_enabled_);
	    		MainRenderer::clear_framebuffers();
	    	}
	    	if(ImGui::Checkbox("2D Batched", &layer2d_enabled_))
	    	{
	    		set_layer_enabled(1, layer2d_enabled_);
	    		presentation_layer_->enable_2d_rendering(layer2d_enabled_);
	    		MainRenderer::clear_framebuffers();
	    	}
	    ImGui::End();
	}

	bool on_keyboard_event(const KeyboardEvent& e)
	{
		// Terminate on ESCAPE
		if(e.pressed && e.key == keymap::WKEY::ESCAPE)
			EVENTBUS.publish(WindowCloseEvent());

		return false;
	}

private:
	bool layer3d_enabled_ = true;
	bool layer2d_enabled_ = false;

	PresentationLayer* presentation_layer_;
};

Application* erwin::create_application()
{
	return new Sandbox();
}
