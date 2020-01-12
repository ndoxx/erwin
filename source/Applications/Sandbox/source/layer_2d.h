#pragma once

#include "erwin.h"

using namespace erwin;

class Layer2D: public Layer
{
public:
	friend class Sandbox;
	
	Layer2D();
	~Layer2D() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;

private:
	OrthographicCamera2DController camera_ctl_;
	TextureAtlasHandle atlas_;
	FontAtlasHandle font_arial_;
	FontAtlasHandle font_zeroprime_;
	std::vector<hash_t> tiles_;
	float tt_ = 0.f;
	bool trippy_mode_ = false;
	int len_grid_ = 100;
};