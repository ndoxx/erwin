#pragma once

#include "erwin.h"
#include "glm/glm.hpp"

namespace erwin
{

struct ComponentRenderablePBR
{
	enum Flags: uint8_t
	{
		MATERIAL_READY = 1<<0,
		MESH_READY = 1<<1,
	};

	VertexArrayHandle vertex_array;
	MaterialHandle material;

	struct MaterialData
	{
		glm::vec4 tint = {1.f,1.f,1.f,1.f};
		int flags = 0;
		float emissive_scale = 1.f;
		float tiling_factor = 1.f;
	} material_data;

	uint8_t flags = 0;

	inline void set_material(MaterialHandle _material)
	{
		material = _material;
		if(material.is_valid())
			flags |= Flags::MATERIAL_READY;
	}

	inline void set_vertex_array(VertexArrayHandle _vertex_array)
	{
		vertex_array = _vertex_array;
		if(vertex_array.is_valid())
			flags |= Flags::MESH_READY;
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
	inline bool is_ready() const    { return bool(flags & Flags::MATERIAL_READY) && bool(flags & Flags::MESH_READY); }
};

template <> [[maybe_unused]] void inspector_GUI<ComponentRenderablePBR>(ComponentRenderablePBR*);


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

template <> [[maybe_unused]] void inspector_GUI<ComponentRenderableDirectionalLight>(ComponentRenderableDirectionalLight*);

} // namespace erwin