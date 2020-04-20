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
		MESH_READY     = 1<<1,
	};

	VertexArrayHandle vertex_array;
	Material material;

	struct MaterialData
	{
		enum Flags
		{
			ENABLE_EMISSIVE  = 1<<0,
			ENABLE_PARALLAX  = 1<<1,
		};

		glm::vec4 tint = {1.f,1.f,1.f,1.f};
		int flags = 0;
		float emissive_scale = 1.f;
		float tiling_factor = 1.f;
		float parallax_height_scale = 0.03f;
	} material_data;

	uint8_t flags = 0;

	inline void set_material(const Material& mat)
	{
		material = mat;
		flags |= Flags::MATERIAL_READY;
	}

	inline void set_vertex_array(VertexArrayHandle _vertex_array)
	{
		vertex_array = _vertex_array;
		if(vertex_array.is_valid())
			flags |= Flags::MESH_READY;
	}

	inline void enable_emissivity(bool enabled = true)
	{
		if(enabled)
			material_data.flags |= MaterialData::Flags::ENABLE_EMISSIVE;
		else
			material_data.flags &= ~MaterialData::Flags::ENABLE_EMISSIVE;
	}

	inline void enable_parallax(bool enabled = true)
	{
		if(enabled)
			material_data.flags |= MaterialData::Flags::ENABLE_PARALLAX;
		else
			material_data.flags &= ~MaterialData::Flags::ENABLE_PARALLAX;
	}

	inline bool is_emissive() const       { return bool(material_data.flags & MaterialData::Flags::ENABLE_EMISSIVE); }
	inline bool has_parallax() const      { return bool(material_data.flags & MaterialData::Flags::ENABLE_PARALLAX); }
	inline bool is_material_ready() const { return bool(flags & Flags::MATERIAL_READY); }
	inline bool is_mesh_ready() const     { return bool(flags & Flags::MESH_READY); }
	inline bool is_ready() const          { return is_material_ready() && is_mesh_ready(); }
};

template <> [[maybe_unused]] void inspector_GUI<ComponentRenderablePBR>(ComponentRenderablePBR*);


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

	inline void set_material(const Material& mat)
	{
		material = mat;
		flags |= Flags::READY;
	}

	inline bool is_ready() const { return bool(flags & Flags::READY); }
};

template <> [[maybe_unused]] void inspector_GUI<ComponentRenderableDirectionalLight>(ComponentRenderableDirectionalLight*);

} // namespace erwin