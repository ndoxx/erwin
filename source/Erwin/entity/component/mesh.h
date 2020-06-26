#pragma once

#include "asset/mesh.h"
#include "render/common_geometry.h"

namespace erwin
{

struct ComponentMesh
{
	Mesh mesh;

	ComponentMesh()
	{
		mesh = CommonGeometry::get_mesh("cube_pbr"_h); // TMP: Can't really default init with a PBR-layout type mesh
	}

	explicit ComponentMesh(const Mesh& me):
	mesh(me)
	{}
};

} // namespace erwin