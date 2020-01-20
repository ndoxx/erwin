#pragma once

#include "erwin.h"
#include "scene.h"

using namespace erwin;

class GameLayer: public Layer
{
public:
	friend class Editor;
	
	GameLayer(editor::Scene& scene);
	~GameLayer() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const MouseButtonEvent& event) override;
	virtual bool on_event(const WindowResizeEvent& event) override;
	virtual bool on_event(const WindowMovedEvent& event) override;
	virtual bool on_event(const MouseScrollEvent& event) override;
	virtual bool on_event(const MouseMovedEvent& event) override;

private:
	ShaderHandle background_shader_;
	PostProcessingData pp_data_;
	editor::Scene& scene_;
};