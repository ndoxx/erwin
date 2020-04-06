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
	MaterialHandle material;

	struct MaterialData
	{
		glm::vec4 tint;
		int flags;
		float emissive_scale;
	} material_data;

	uint8_t flags = 0;

	inline void set_material(MaterialHandle _material)
	{
		material = _material;
		if(material.is_valid())
			flags |= Flags::READY;
	}

	inline void set_emissive(float intensity)
	{
		material_data.flags |= (1<<0);
		material_data.emissive_scale = intensity;
	}

	inline void disable_emissivity()
	{
		material_data.flags &= ~(1<<0);
		material_data.emissive_scale = 0.f;
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

	MaterialHandle material;

	struct MaterialData
	{
		glm::vec4 color;
		float scale;
		float brightness;
	} material_data;

	uint8_t flags = 0;

	inline void set_material(MaterialHandle _material)
	{
		material = _material;
		if(material.is_valid())
			flags |= Flags::READY;
	}

	inline bool is_ready() const { return bool(flags & Flags::READY); }
};

template <> [[maybe_unused]] void inspector_GUI<ComponentRenderableDirectionalLight>(void* data);

} // namespace erwin