#pragma once

#include "glm/glm.hpp"
#include "core/core.h"
#include "render/handles.h"

namespace erwin
{

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

enum ClearFlags: uint8_t
{
    CLEAR_NONE = 0,
    CLEAR_COLOR_FLAG = 1,
    CLEAR_DEPTH_FLAG = 2,
    CLEAR_STENCIL_FLAG = 4
};

struct RasterizerState
{
    uint8_t clear_flags    = ClearFlags::CLEAR_NONE;
    CullMode cull_mode     = CullMode::None;
    glm::vec4 clear_color  = glm::vec4(0.f,0.f,0.f,0.f);
};

struct DepthStencilState
{
    DepthFunc       depth_func       = DepthFunc::Less;
    StencilFunc     stencil_func     = StencilFunc::Always;
    StencilOperator stencil_operator = StencilOperator::LightVolume;
    bool       stencil_test_enabled  = false;
    bool       depth_test_enabled    = false;
    bool       depth_lock            = false;
    bool       stencil_lock          = false;
};

// Render target
constexpr uint8_t  k_framebuffer_bits   = 16;
constexpr uint64_t k_framebuffer_shift  = uint64_t(64) - k_framebuffer_bits;
constexpr uint64_t k_framebuffer_mask   = uint64_t(0xffff) << k_framebuffer_shift;
// Target mip level
constexpr uint8_t  k_target_mips_bits   = 3;
constexpr uint64_t k_target_mips_shift  = k_framebuffer_shift - k_target_mips_bits;
constexpr uint64_t k_target_mips_mask   = uint64_t(0x7) << k_target_mips_shift;
// Clear flags
constexpr uint8_t  k_clear_bits         = 3;
constexpr uint64_t k_clear_shift        = k_target_mips_shift - k_clear_bits;
constexpr uint64_t k_clear_mask         = uint64_t(0x7) << k_clear_shift;
// Transparency
constexpr uint8_t  k_transp_bits        = 2;
constexpr uint64_t k_transp_shift       = k_clear_shift - k_transp_bits;
constexpr uint64_t k_transp_mask        = uint64_t(0x3) << k_transp_shift;
// Cull mode (none / front / back)
constexpr uint8_t  k_cull_mode_bits     = 2;
constexpr uint64_t k_cull_mode_shift    = k_transp_shift - k_cull_mode_bits;
constexpr uint64_t k_cull_mode_mask     = uint64_t(0x3) << k_cull_mode_shift;
// Depth and stencil test enable bits
constexpr uint64_t k_depth_test_shift   = k_cull_mode_shift - 1;
constexpr uint64_t k_stencil_test_shift = k_depth_test_shift - 1;
constexpr uint64_t k_depth_test_mask    = uint64_t(1) << k_depth_test_shift;
constexpr uint64_t k_stencil_test_mask  = uint64_t(1) << k_stencil_test_shift;
// Depth and stencil lock bits
constexpr uint64_t k_depth_lock_shift   = k_stencil_test_shift - 1;
constexpr uint64_t k_stencil_lock_shift = k_depth_lock_shift - 1;
constexpr uint64_t k_depth_lock_mask    = uint64_t(1) << k_depth_lock_shift;
constexpr uint64_t k_stencil_lock_mask  = uint64_t(1) << k_stencil_lock_shift;
// Depth state
constexpr uint8_t  k_depth_func_bits    = 3;
constexpr uint64_t k_depth_func_shift   = k_stencil_lock_shift - k_depth_func_bits;
constexpr uint64_t k_depth_func_mask    = uint64_t(0x7) << k_depth_func_shift;
// Stencil state
constexpr uint8_t  k_stencil_func_bits  = 3;
constexpr uint64_t k_stencil_func_shift = k_depth_func_shift - k_stencil_func_bits;
constexpr uint64_t k_stencil_func_mask  = uint64_t(0x7) << k_stencil_func_shift;
constexpr uint8_t  k_stencil_op_bits    = 3;
constexpr uint64_t k_stencil_op_shift   = k_stencil_func_shift - k_stencil_op_bits;
constexpr uint64_t k_stencil_op_mask    = uint64_t(0x7) << k_stencil_op_shift;

struct RenderState
{
    BlendState        blend_state    = BlendState::Opaque;
    RasterizerState   rasterizer_state;
    DepthStencilState depth_stencil_state;
    uint16_t          render_target;
    uint8_t           target_mip_level = 0;

    inline uint64_t encode() const
    {
        return ((static_cast<uint64_t>(render_target)                            << k_framebuffer_shift)  & k_framebuffer_mask)
             | ((static_cast<uint64_t>(target_mip_level)                         << k_target_mips_shift)  & k_target_mips_mask)
             | ((static_cast<uint64_t>(rasterizer_state.clear_flags)             << k_clear_shift)        & k_clear_mask)
             | ((static_cast<uint64_t>(blend_state)                              << k_transp_shift)       & k_transp_mask)
             | ((static_cast<uint64_t>(rasterizer_state.cull_mode)               << k_cull_mode_shift)    & k_cull_mode_mask)
             | ((static_cast<uint64_t>(depth_stencil_state.depth_test_enabled)   << k_depth_test_shift)   & k_depth_test_mask)
             | ((static_cast<uint64_t>(depth_stencil_state.stencil_test_enabled) << k_stencil_test_shift) & k_stencil_test_mask)
             | ((static_cast<uint64_t>(depth_stencil_state.depth_lock)           << k_depth_lock_shift)   & k_depth_lock_mask)
             | ((static_cast<uint64_t>(depth_stencil_state.stencil_lock)         << k_stencil_lock_shift) & k_stencil_lock_mask)
             | ((static_cast<uint64_t>(depth_stencil_state.depth_func)           << k_depth_func_shift)   & k_depth_func_mask)
             | ((static_cast<uint64_t>(depth_stencil_state.stencil_func)         << k_stencil_func_shift) & k_stencil_func_mask)
             | ((static_cast<uint64_t>(depth_stencil_state.stencil_operator)     << k_stencil_op_shift)   & k_stencil_op_mask);
    }

    inline void decode(uint64_t state)
    {
        render_target                            = uint16_t(       (state & k_framebuffer_mask)  >> k_framebuffer_shift);
        target_mip_level                         = uint8_t(        (state & k_target_mips_mask)  >> k_target_mips_shift);
        rasterizer_state.clear_flags             = uint8_t(        (state & k_clear_mask)        >> k_clear_shift);
        blend_state                              = BlendState(     (state & k_transp_mask)       >> k_transp_shift);
        rasterizer_state.cull_mode               = CullMode(       (state & k_cull_mode_mask)    >> k_cull_mode_shift);
        depth_stencil_state.depth_test_enabled   = bool(           (state & k_depth_test_mask)   >> k_depth_test_shift);
        depth_stencil_state.stencil_test_enabled = bool(           (state & k_stencil_test_mask) >> k_stencil_test_shift);
        depth_stencil_state.depth_lock           = bool(           (state & k_depth_lock_mask)   >> k_depth_lock_shift);
        depth_stencil_state.stencil_lock         = bool(           (state & k_stencil_lock_mask) >> k_stencil_lock_shift);
        depth_stencil_state.depth_func           = DepthFunc(      (state & k_depth_func_mask)   >> k_depth_func_shift);
        depth_stencil_state.stencil_func         = StencilFunc(    (state & k_stencil_func_mask) >> k_stencil_func_shift);
        depth_stencil_state.stencil_operator     = StencilOperator((state & k_stencil_op_mask)   >> k_stencil_op_shift);
    }

    static inline bool is_transparent(uint64_t pass_state)
    {
        return bool((pass_state & k_transp_mask) >> k_transp_shift);
    }

    std::string to_string();
};

} // namespace erwin