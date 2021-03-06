#pragma once

#include <cstdint>
#include <kibble/hash/hash.h>
#include "render/handles.h"

namespace erwin
{

struct Environment
{
    CubemapHandle environment_map = {};
    CubemapHandle diffuse_irradiance_map = {};
    CubemapHandle prefiltered_map = {};

    uint32_t size = 0;
	bool IBL_enabled = true;
	float ambient_strength = 0.15f;
	hash_t resource_id = 0;
};


} // namespace erwin