#pragma once

#include "erwin.h"

class GameLayer: public erwin::Layer
{
public:
	friend class Editor;
	
	GameLayer();
	~GameLayer() = default;

	virtual void on_imgui_render() override;
	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_update(erwin::GameClock& clock) override;
	virtual void on_render() override;
	virtual bool on_event(const erwin::MouseButtonEvent& event) override;
	virtual bool on_event(const erwin::WindowResizeEvent& event) override;
	virtual bool on_event(const erwin::WindowMovedEvent& event) override;
	virtual bool on_event(const erwin::MouseScrollEvent& event) override;
	virtual bool on_event(const erwin::MouseMovedEvent& event) override;
};