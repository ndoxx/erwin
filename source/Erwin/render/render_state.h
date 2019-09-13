#pragma once

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

enum class RenderTarget
{
    Default = 0,
    GBuffer,
};

typedef CullMode RasterizerState;

struct DepthStencilState
{
    DepthFunc       depth_func       = DepthFunc::Less;
    StencilFunc     stencil_func     = StencilFunc::Always;
    StencilOperator stencil_operator = StencilOperator::LightVolume;
    bool       stencil_test_enabled  = false;
    bool       depth_test_enabled    = false;
};

struct RenderState
{
    RenderTarget      render_target       = RenderTarget::Default;
    RasterizerState   rasterizer_state    = CullMode::Back;
    BlendState        blend_state         = BlendState::Opaque;
    DepthStencilState depth_stencil_state;
};

} // namespace erwin