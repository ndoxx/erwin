#pragma once

#include "asset/mesh.h"

namespace erwin
{

struct ComponentMesh
{
	Mesh mesh;
	bool ready = false;

	inline void init(const Mesh& mesh_)
	{
		mesh = mesh_;
		if(mesh.VAO.is_valid())
			ready = true;
	}

	inline bool is_ready() const { return ready; }
};

} // namespace erwin