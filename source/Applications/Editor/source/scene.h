#pragma once

#include "freefly_camera_controller.h"

namespace editor
{

class Scene
{
public:
	Scene();

	FreeflyController camera_controller;
};

} // namespace editor