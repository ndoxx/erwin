#pragma once

#include "erwin.h"
#include "glm/glm.hpp"

namespace erwin
{

struct ComponentRenderablePBR
{
	enum Flags: uint8_t
	{
		READY = 1<<0,
	};

	VertexArrayHandle vertex_array;
	Material material;

	struct MaterialData
	{
		glm::vec4 tint;
		int flags;
		float emissive_scale;
	} material_data;

	uint8_t flags = 0;

	inline void set_material(const Material& _material)
	{
		material = _material;
		material.data_size = sizeof(MaterialData);
		if(material.shader.is_valid() && material.texture_group.is_valid() && material.ubo.is_valid())
			flags |= Flags::READY;
	}

	inline void set_emissive(float intensity)
	{
		material_data.flags |= (1<<0);
		material_data.emissive_scale = intensity;
	}

	inline bool is_emissive() const { return bool(material_data.flags & (1<<0)); }
	inline bool is_ready() const    { return bool(flags & Flags::READY); }
};

template <> [[maybe_unused]] void inspector_GUI<ComponentRenderablePBR>(void* data);


struct ComponentRenderableDirectionalLight
{
	enum Flags: uint8_t
	{
		READY = 1<<0,
	};

	Material material;

	struct MaterialData
	{
		glm::vec4 color;
		float scale;
		float brightness;
	} material_data;

	uint8_t flags = 0;

	inline void set_material(const Material& _material)
	{
		material = _material;
		material.data_size = sizeof(MaterialData);
		if(material.shader.is_valid() && material.ubo.is_valid())
			flags |= Flags::READY;
	}

	inline bool is_ready() const { return bool(flags & Flags::READY); }
};

template <> [[maybe_unused]] void inspector_GUI<ComponentRenderableDirectionalLight>(void* data);

} // namespace erwin