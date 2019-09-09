#pragma once

#include "../core/layer.h"

namespace erwin
{

class ImGuiLayer: public Layer
{
public:
	ImGuiLayer();
	~ImGuiLayer();

	virtual void on_attach() override;
	virtual void on_detach() override;
	virtual void on_imgui_render() override;

	void begin();
	void end();

protected:
	virtual void on_update() override {}
};


} // namespace erwin