#pragma once

#include "glm/glm.hpp"
#include "core/wtypes.h"
#include "render/handles.h"

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
    Alpha = 1,
    Light = 2
};

enum ClearFlags
{
    CLEAR_NONE = 0,
    CLEAR_COLOR_FLAG = 1,
    CLEAR_DEPTH_FLAG = 2,
    CLEAR_STENCIL_FLAG = 4
};

struct RasterizerState
{
    ClearFlags clear_flags = ClearFlags::CLEAR_NONE;
    CullMode cull_mode     = CullMode::Back;
    glm::vec4 clear_color  = glm::vec4(0.f,0.f,0.f,1.f);
};

struct DepthStencilState
{
    DepthFunc       depth_func       = DepthFunc::Less;
    StencilFunc     stencil_func     = StencilFunc::Always;
    StencilOperator stencil_operator = StencilOperator::LightVolume;
    bool       stencil_test_enabled  = false;
    bool       depth_test_enabled    = false;
};

// Render target
constexpr uint8_t  k_framebuffer_bits   = 16;
constexpr uint64_t k_framebuffer_shift  = uint64_t(64) - k_framebuffer_bits;
constexpr uint64_t k_framebuffer_mask   = uint64_t(0xffff) << k_framebuffer_shift;
// Transparency
constexpr uint8_t  k_transp_bits        = 2;
constexpr uint64_t k_transp_shift       = k_framebuffer_shift - k_transp_bits;
constexpr uint64_t k_transp_mask        = uint64_t(0x3) << k_transp_shift;
// Cull mode (none / front / back)
constexpr uint8_t  k_cull_mode_bits     = 2;
constexpr uint64_t k_cull_mode_shift    = k_transp_shift - k_cull_mode_bits;
constexpr uint64_t k_cull_mode_mask     = uint64_t(0x3) << k_cull_mode_shift;
// Depth and stencil test enable bit
constexpr uint64_t k_depth_test_shift   = k_cull_mode_shift - 1;
constexpr uint64_t k_stencil_test_shift = k_depth_test_shift - 1;
constexpr uint64_t k_depth_test_mask    = uint64_t(1) << k_depth_test_shift;
constexpr uint64_t k_stencil_test_mask  = uint64_t(1) << k_stencil_test_shift;
// Depth state
constexpr uint8_t  k_depth_func_bits    = 3;
constexpr uint64_t k_depth_func_shift   = k_stencil_test_shift - k_depth_func_bits;
constexpr uint64_t k_depth_func_mask    = uint64_t(0x7) << k_depth_func_shift;
// Stencil state
constexpr uint8_t  k_stencil_func_bits  = 3;
constexpr uint64_t k_stencil_func_shift = k_depth_func_shift - k_stencil_func_bits;
constexpr uint64_t k_stencil_func_mask  = uint64_t(0x7) << k_stencil_func_shift;
constexpr uint8_t  k_stencil_op_bits    = 3;
constexpr uint64_t k_stencil_op_shift   = k_stencil_func_shift - k_stencil_op_bits;
constexpr uint64_t k_stencil_op_mask    = uint64_t(0x7) << k_stencil_op_shift;

struct PassState
{
    BlendState        blend_state    = BlendState::Opaque;
    RasterizerState   rasterizer_state;
    DepthStencilState depth_stencil_state;
    FramebufferHandle render_target;

    inline uint64_t encode() const
    {
        return (((uint64_t)render_target.index                      << k_framebuffer_shift)  & k_framebuffer_mask)
             | (((uint64_t)blend_state                              << k_transp_shift)       & k_transp_mask)
             | (((uint64_t)rasterizer_state.cull_mode               << k_cull_mode_shift)    & k_cull_mode_mask)
             | (((uint64_t)depth_stencil_state.depth_test_enabled   << k_depth_test_shift)   & k_depth_test_mask)
             | (((uint64_t)depth_stencil_state.stencil_test_enabled << k_stencil_test_shift) & k_stencil_test_mask)
             | (((uint64_t)depth_stencil_state.depth_func           << k_depth_func_shift)   & k_depth_func_mask)
             | (((uint64_t)depth_stencil_state.stencil_func         << k_stencil_func_shift) & k_stencil_func_mask)
             | (((uint64_t)depth_stencil_state.stencil_operator     << k_stencil_op_shift)   & k_stencil_op_mask);
    }

    inline void decode(uint64_t state)
    {
        render_target.index                      = uint16_t(       (state & k_framebuffer_mask)  >> k_framebuffer_shift);
        blend_state                              = BlendState(     (state & k_transp_mask)       >> k_transp_shift);
        rasterizer_state.cull_mode               = CullMode(       (state & k_cull_mode_mask)    >> k_cull_mode_shift);
        depth_stencil_state.depth_test_enabled   = bool(           (state & k_depth_test_mask)   >> k_depth_test_shift);
        depth_stencil_state.stencil_test_enabled = bool(           (state & k_stencil_test_mask) >> k_stencil_test_shift);
        depth_stencil_state.depth_func           = DepthFunc(      (state & k_depth_func_mask)   >> k_depth_func_shift);
        depth_stencil_state.stencil_func         = StencilFunc(    (state & k_stencil_func_mask) >> k_stencil_func_shift);
        depth_stencil_state.stencil_operator     = StencilOperator((state & k_stencil_op_mask)   >> k_stencil_op_shift);
    }

    static inline bool is_transparent(uint64_t pass_state)
    {
        return bool((pass_state & k_transp_mask) >> k_transp_shift);
    }
};

} // namespace erwin