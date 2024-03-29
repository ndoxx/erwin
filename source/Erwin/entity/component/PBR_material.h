#pragma once

#include "asset/material.h"
#include "glm/glm.hpp"
#include "render/texture_common.h"

namespace erwin
{

struct ComponentPBRMaterial
{
    Material material;

    struct MaterialData
    {
        glm::vec4 tint = {1.f, 1.f, 1.f, 1.f};
        int flags = 0;
        float emissive_scale = 1.f;
        float tiling_factor = 1.f;
        float parallax_height_scale = 0.03f;

        // Uniform maps
        glm::vec4 uniform_albedo = {1.f, 1.f, 1.f, 1.f};
        float uniform_metallic = 0.f;
        float uniform_roughness = 0.5f;
    } material_data;

    std::string name;
    bool override = false;

    ComponentPBRMaterial() = default;

    ComponentPBRMaterial(const Material& mat, const MaterialData& data) : material(mat), material_data(data) {}

    ComponentPBRMaterial(const Material& mat, const void* p_material_data) : material(mat)
    {
        memcpy(&material_data, p_material_data, sizeof(MaterialData));
    }

    inline void enable_flag(TextureMapFlag flag, bool enabled = true)
    {
        if(enabled)
            material_data.flags |= flag;
        else
            material_data.flags &= ~flag;
    }

    inline void clear_flags() { material_data.flags = 0; }

    inline void enable_albedo_map(bool enabled = true) { enable_flag(TextureMapFlag::TMF_ALBEDO, enabled); }
    inline void enable_normal_map(bool enabled = true) { enable_flag(TextureMapFlag::TMF_NORMAL, enabled); }
    inline void enable_parallax(bool enabled = true) { enable_flag(TextureMapFlag::TMF_DEPTH, enabled); }
    inline void enable_metallic_map(bool enabled = true) { enable_flag(TextureMapFlag::TMF_METAL, enabled); }
    inline void enable_ao_map(bool enabled = true) { enable_flag(TextureMapFlag::TMF_AO, enabled); }
    inline void enable_roughness_map(bool enabled = true) { enable_flag(TextureMapFlag::TMF_ROUGHNESS, enabled); }
    inline void enable_emissivity(bool enabled = true) { enable_flag(TextureMapFlag::TMF_EMISSIVITY, enabled); }
    inline bool is_emissive() const { return bool(material_data.flags & TextureMapFlag::TMF_EMISSIVITY); }
    inline bool has_parallax() const { return bool(material_data.flags & TextureMapFlag::TMF_DEPTH); }
};

} // namespace erwin