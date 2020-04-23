#pragma once

#include "entity/reflection.h"
#include "asset/material.h"
#include "glm/glm.hpp"

namespace erwin
{

struct ComponentDirectionalLightMaterial
{
	Material material;

	struct MaterialData
	{
		glm::vec4 color;
		float scale;
		float brightness;
	} material_data;

	bool ready = false;

	inline void set_material(const Material& mat)
	{
		material = mat;
		ready = true;
	}

	inline bool is_ready() const { return ready; }
};

template <> [[maybe_unused]] void inspector_GUI<ComponentDirectionalLightMaterial>(ComponentDirectionalLightMaterial*);

} // namespace erwin