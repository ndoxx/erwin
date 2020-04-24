#pragma once

#include "entity/reflection.h"
#include "asset/material.h"
#include "glm/glm.hpp"


namespace erwin
{

struct ComponentPBRMaterial
{
	Material material;

	struct MaterialData
	{
		enum Flags
		{
			ENABLE_EMISSIVE      = 1<<0,
			ENABLE_PARALLAX      = 1<<1,
			ENABLE_ALBEDO_MAP    = 1<<2,
			ENABLE_NORMAL_MAP    = 1<<3,
			ENABLE_METALLIC_MAP  = 1<<4,
			ENABLE_AO_MAP        = 1<<5,
			ENABLE_ROUGHNESS_MAP = 1<<6,
		};

		glm::vec4 tint = {1.f,1.f,1.f,1.f};
		int flags = 0;
		float emissive_scale = 1.f;
		float tiling_factor = 1.f;
		float parallax_height_scale = 0.03f;

		// Uniform maps
		glm::vec4 uniform_albedo = {1.f,1.f,1.f,1.f};
		float uniform_metallic   = 0.f;
		float uniform_roughness  = 0.5f;
	} material_data;

	bool ready = false;

	inline void set_material(const Material& mat)
	{
		material = mat;
		ready = true;
	}

	inline void enable_flag(MaterialData::Flags flag, bool enabled = true)
	{
		if(enabled)
			material_data.flags |= flag;
		else
			material_data.flags &= ~flag;
	}

	inline void enable_emissivity(bool enabled = true)    { enable_flag(MaterialData::Flags::ENABLE_EMISSIVE, enabled); }
	inline void enable_parallax(bool enabled = true)      { enable_flag(MaterialData::Flags::ENABLE_PARALLAX, enabled); }
	inline void enable_albedo_map(bool enabled = true)    { enable_flag(MaterialData::Flags::ENABLE_ALBEDO_MAP, enabled); }
	inline void enable_normal_map(bool enabled = true)    { enable_flag(MaterialData::Flags::ENABLE_NORMAL_MAP, enabled); }
	inline void enable_metallic_map(bool enabled = true)  { enable_flag(MaterialData::Flags::ENABLE_METALLIC_MAP, enabled); }
	inline void enable_ao_map(bool enabled = true)        { enable_flag(MaterialData::Flags::ENABLE_AO_MAP, enabled); }
	inline void enable_roughness_map(bool enabled = true) { enable_flag(MaterialData::Flags::ENABLE_ROUGHNESS_MAP, enabled); }
	inline bool is_emissive() const       				  { return bool(material_data.flags & MaterialData::Flags::ENABLE_EMISSIVE); }
	inline bool has_parallax() const      				  { return bool(material_data.flags & MaterialData::Flags::ENABLE_PARALLAX); }
	inline bool is_ready() const          				  { return ready; }
};

template <> [[maybe_unused]] void inspector_GUI<ComponentPBRMaterial>(ComponentPBRMaterial*);


} // namespace erwin