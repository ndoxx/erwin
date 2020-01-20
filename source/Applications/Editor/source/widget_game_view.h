#pragma once

#include "widget.h"

namespace editor
{

class GameViewWidget: public Widget
{
public:
	GameViewWidget();
	virtual ~GameViewWidget();

protected:
	virtual void on_render() override;
	virtual void on_resize(uint32_t width, uint32_t height) override;

private:

};

} // namespace editor