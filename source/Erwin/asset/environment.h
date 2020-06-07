#pragma once

#include <cstdint>
#include "render/handles.h"

namespace erwin
{

struct Environment
{
    CubemapHandle environment_map;
    CubemapHandle diffuse_irradiance_map;
    CubemapHandle prefiltered_env_map;

    uint32_t size;
};


} // namespace erwin