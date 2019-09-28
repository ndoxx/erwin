#pragma once

#include "erwin.h"
using namespace erwin;


class LayerTest: public Layer
{
public:
	LayerTest();
	~LayerTest() = default;

	virtual void on_imgui_render() override
	{

	}

	virtual void on_attach() override
	{

	}


protected:
	virtual void on_update(GameClock& clock) override
	{

	}


private:

};