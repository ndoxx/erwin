#pragma once

#include "erwin.h"

using namespace erwin;

class PresentationLayer: public Layer
{
public:
	friend class Sandbox;

	PresentationLayer();
	~PresentationLayer() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

	inline void enable_forward_rendering(bool value) { enable_3d_forward_ = value; }
	inline void enable_bloom(bool value) { enable_bloom_ = value; }
	inline void enable_2d_rendering(bool value) { enable_2d_batched_ = value; }

protected:
	virtual void on_update(GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;

private:
	float tt_ = 0.f;
	bool enable_3d_forward_ = true;
	bool enable_2d_batched_ = false;

	bool enable_bloom_ = true;
	bool enable_chromatic_aberration_ = true;
	bool enable_exposure_tone_mapping_ = true;
	bool enable_vibrance_ = true;
	bool enable_saturation_ = true;
	bool enable_contrast_ = true;
	bool enable_gamma_ = true;
	bool enable_fxaa_ = true;
	PostProcessingData pp_data_;
};