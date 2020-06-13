#pragma once

#include "render/handles.h"
#include "asset/bounding.h"
#include "asset/mesh.h"

namespace erwin
{

struct ComponentMesh
{
	VertexArrayHandle vertex_array;
	Extent extent = {-0.5f,0.5f,-0.5f,0.5f,-0.5f,0.5f};
	bool ready = false;

	inline void init(const Mesh& mesh)
	{
		vertex_array = mesh.VAO;
		extent = mesh.extent;
		if(vertex_array.is_valid())
			ready = true;
	}

	inline bool is_ready() const { return ready; }
};

} // namespace erwin