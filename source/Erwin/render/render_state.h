#pragma once

#include "glm/glm.hpp"
#include "core/wtypes.h"

namespace erwin
{

// Describes data types held in buffer layouts and passed to shaders
enum class ShaderDataType: uint8_t
{
    Float = 0, Vec2, Vec3, Vec4, Mat3, Mat4, Int, IVec2, IVec3, IVec4, Bool
};

enum class DrawMode: uint8_t
{
    Static = 0, Stream, Dynamic
};

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

// Transparency bit
constexpr uint64_t k_shift_transp_bit = uint8_t(64) - 1;
// Cull mode (none / front / back)
constexpr uint8_t  k_bits_cull_mode = 2;
constexpr uint64_t k_shift_cull_mode = k_shift_transp_bit - k_bits_cull_mode;
// Depth and stencil test enable bit
constexpr uint64_t k_shift_depth_test_bit = k_shift_cull_mode - 1;
constexpr uint64_t k_shift_stencil_test_bit = k_shift_depth_test_bit - 1;
// Depth state
constexpr uint8_t  k_bits_depth_func = 3;
constexpr uint64_t k_shift_depth_func = k_shift_stencil_test_bit - k_bits_depth_func;
// Stencil state
constexpr uint8_t  k_bits_stencil_func = 3;
constexpr uint64_t k_shift_stencil_func = k_shift_depth_func - k_bits_stencil_func;
constexpr uint8_t  k_bits_stencil_op = 3;
constexpr uint64_t k_shift_stencil_op = k_shift_stencil_func - k_bits_stencil_op;

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

    inline uint64_t encode() const
    {
        return (uint64_t)blend_state                              << k_shift_transp_bit
             | (uint64_t)rasterizer_state.cull_mode               << k_shift_cull_mode
             | (uint64_t)depth_stencil_state.depth_test_enabled   << k_shift_depth_test_bit
             | (uint64_t)depth_stencil_state.stencil_test_enabled << k_shift_stencil_test_bit
             | (uint64_t)depth_stencil_state.depth_func           << k_shift_depth_func
             | (uint64_t)depth_stencil_state.stencil_func         << k_shift_stencil_func
             | (uint64_t)depth_stencil_state.stencil_operator     << k_shift_stencil_op;
    }
};

} // namespace erwin