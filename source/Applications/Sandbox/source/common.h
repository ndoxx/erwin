#pragma once

#include "erwin.h"

using namespace erwin;

struct PBRMaterialData
{
	inline void enable_emissivity() { flags |= (1<<0); }

	glm::vec4 tint;
	int flags;
	float emissive_scale;
};

struct Cube
{
	ComponentTransform3D transform;
	Material material;
	PBRMaterialData material_data;
};