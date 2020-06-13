#pragma once

#include "render/handles.h"
#include "asset/bounding.h"

namespace erwin
{

struct ComponentMesh
{
	VertexArrayHandle vertex_array;
	Extent extent = {-0.5f,0.5f,-0.5f,0.5f,-0.5f,0.5f};
	bool ready = false;

	inline void set_vertex_array(VertexArrayHandle vao)
	{
		vertex_array = vao;
		if(vertex_array.is_valid())
			ready = true;
	}

	inline bool is_ready() const { return ready; }
};

} // namespace erwin