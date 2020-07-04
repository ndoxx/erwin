#pragma once

#include "asset/material.h"
#include "render/texture_common.h"
#include "asset/asset_manager.h"
#include "glm/glm.hpp"

namespace erwin
{

struct ComponentPBRMaterial
{
	Material material;

	struct MaterialData
	{
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

	std::string name;
	bool override = false;

	ComponentPBRMaterial()
	{
		material.shader = AssetManager::load_shader("shaders/deferred_PBR.glsl");
		material.ubo    = AssetManager::create_material_data_buffer<ComponentPBRMaterial>();
		material.data_size = sizeof(MaterialData);
	}

	explicit ComponentPBRMaterial(const Material& mat):
	material(mat)
	{
		material.data_size = sizeof(MaterialData);
	}

	ComponentPBRMaterial(const Material& mat, const MaterialData& data):
	material(mat),
	material_data(data)
	{
		material.data_size = sizeof(MaterialData);
	}

	ComponentPBRMaterial(const Material& mat, const void* p_material_data):
	material(mat)
	{
    	memcpy(&material_data, p_material_data, sizeof(MaterialData));
		material.data_size = sizeof(MaterialData);
	}

	inline void enable_flag(TextureMapFlag flag, bool enabled = true)
	{
		if(enabled)
			material_data.flags |= flag;
		else
			material_data.flags &= ~flag;
	}

	inline void clear_flags()
	{
		material_data.flags = 0;
	}

	inline void enable_albedo_map(bool enabled = true)    { enable_flag(TextureMapFlag::TMF_ALBEDO, enabled); }
	inline void enable_normal_map(bool enabled = true)    { enable_flag(TextureMapFlag::TMF_NORMAL, enabled); }
	inline void enable_parallax(bool enabled = true)      { enable_flag(TextureMapFlag::TMF_DEPTH, enabled); }
	inline void enable_metallic_map(bool enabled = true)  { enable_flag(TextureMapFlag::TMF_METAL, enabled); }
	inline void enable_ao_map(bool enabled = true)        { enable_flag(TextureMapFlag::TMF_AO, enabled); }
	inline void enable_roughness_map(bool enabled = true) { enable_flag(TextureMapFlag::TMF_ROUGHNESS, enabled); }
	inline void enable_emissivity(bool enabled = true)    { enable_flag(TextureMapFlag::TMF_EMISSIVITY, enabled); }
	inline bool is_emissive() const       				  { return bool(material_data.flags & TextureMapFlag::TMF_EMISSIVITY); }
	inline bool has_parallax() const      				  { return bool(material_data.flags & TextureMapFlag::TMF_DEPTH); }
};

} // namespace erwin