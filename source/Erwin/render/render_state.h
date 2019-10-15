#pragma once

#include "glm/glm.hpp"
#include "core/wtypes.h"

namespace erwin
{

enum class DrawPrimitive
{
    Lines = 2,
    Triangles = 3,
    Quads = 4
};

enum class CullMode
{
    None = 0,
    Front = 1,
    Back = 2
};

enum class DepthFunc
{
    Less = 0,
    LEqual = 1
};

enum class StencilFunc
{
    Always = 0,
    NotEqual = 1
};

enum class StencilOperator
{
    LightVolume = 0
};

enum class BlendState
{
    Opaque = 0,
    Alpha = 1
};

enum ClearFlags
{
    CLEAR_COLOR_FLAG = 1,
    CLEAR_DEPTH_FLAG = 2,
    CLEAR_STENCIL_FLAG = 4
};

struct RasterizerState
{
    CullMode cull_mode    = CullMode::Back;
    glm::vec4 clear_color = glm::vec4(0.f,0.f,0.f,1.f);
};

struct DepthStencilState
{
    DepthFunc       depth_func       = DepthFunc::Less;
    StencilFunc     stencil_func     = StencilFunc::Always;
    StencilOperator stencil_operator = StencilOperator::LightVolume;
    bool       stencil_test_enabled  = false;
    bool       depth_test_enabled    = false;
};

struct PassState
{
    hash_t            render_target  = 0; // Name of framebuffer in FramebufferPool
    BlendState        blend_state    = BlendState::Opaque;
    RasterizerState   rasterizer_state;
    DepthStencilState depth_stencil_state;

    inline void reset()
    {
        render_target       = 0;
        blend_state         = BlendState::Opaque;
        rasterizer_state    = RasterizerState();
        depth_stencil_state = DepthStencilState();
    }
};

} // namespace erwin