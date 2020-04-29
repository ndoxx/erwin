#pragma once

#include "erwin.h"

namespace editor
{

class EditorBackgroundLayer: public erwin::Layer
{
public:
	EditorBackgroundLayer();

	virtual void on_attach() override;
	virtual void on_detach() override;

protected:
	virtual void on_render() override;

private:
	erwin::ShaderHandle background_shader_;
};

} // namespace editor
