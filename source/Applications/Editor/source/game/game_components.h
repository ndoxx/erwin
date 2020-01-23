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


class ComponentRenderableDirectionalLight: public Component
{
public:
	COMPONENT_DECLARATION(ComponentRenderableDirectionalLight);

	ComponentRenderableDirectionalLight();

	virtual bool init(void* description) override final;

	Material material;

	struct MaterialData
	{
		glm::vec4 color;
		float scale;
		float brightness;
	} material_data;
};

} // namespace erwin