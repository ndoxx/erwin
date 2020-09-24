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
	hash_t resource_id;
	bool procedural = false;
};

} // namespace erwin