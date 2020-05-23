#pragma once

#include "render/handles.h"

namespace erwin
{

struct ComponentMesh
{
	VertexArrayHandle vertex_array;
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