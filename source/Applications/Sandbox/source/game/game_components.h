#pragma once

#include "erwin.h"
#include "glm/glm.hpp"

namespace erwin
{

struct ComponentRenderablePBR
{
	VertexArrayHandle vertex_array;
	Material material;

	struct MaterialData
	{
		glm::vec4 tint;
		int flags;
		float emissive_scale;
	} material_data;


	ComponentRenderablePBR();

	inline void set_emissive(float intensity)
	{
		material_data.flags |= (1<<0);
		material_data.emissive_scale = intensity;
	}

	inline bool is_emissive() const
	{
		return bool(material_data.flags & (1<<0));
	}
};

template <> [[maybe_unused]] void inspector_GUI<ComponentRenderablePBR>(void* data);


struct ComponentRenderableDirectionalLight
{
	Material material;

	struct MaterialData
	{
		glm::vec4 color;
		float scale;
		float brightness;
	} material_data;


	ComponentRenderableDirectionalLight();
};

template <> [[maybe_unused]] void inspector_GUI<ComponentRenderableDirectionalLight>(void* data);

} // namespace erwin