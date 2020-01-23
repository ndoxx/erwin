#pragma once

#include "erwin.h"
#include "glm/glm.hpp"

namespace erwin
{

class ComponentRenderablePBRDeferred: public Component
{
public:
	COMPONENT_DECLARATION(ComponentRenderablePBRDeferred);

	ComponentRenderablePBRDeferred();

	virtual bool init(void* description) override final;
	inline void set_emissive(float intensity)
	{
		material_data.flags |= (1<<0);
		material_data.emissive_scale = intensity;
		material.data_size = sizeof(MaterialData);
	}

	VertexArrayHandle vertex_array;
	Material material;

	struct MaterialData
	{
		glm::vec4 tint;
		int flags;
		float emissive_scale;
	} material_data;
};

} // namespace erwin