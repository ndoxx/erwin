#pragma once

#include "render/handles.h"
#include "asset/bounding.h"

namespace erwin
{

struct Mesh
{
	VertexArrayHandle VAO;
	VertexBufferLayoutHandle layout;
	Extent extent;
	char name[32];
};

} // namespace erwin