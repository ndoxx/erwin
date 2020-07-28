#pragma once

#include "glm/glm.hpp"

namespace editor
{

struct ComponentGizmo
{
	glm::mat4 model_matrix;
	glm::mat4 delta;
};


} // namespace editor