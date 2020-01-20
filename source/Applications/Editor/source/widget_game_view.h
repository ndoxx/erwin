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
	virtual void on_move(int32_t x_pos, int32_t y_pos) override;

private:

};

} // namespace editor