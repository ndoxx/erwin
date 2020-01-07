#pragma once

#include "core/wtypes.h"
#include "render/camera_3d.h"
#include "entity/component_transform.h"
#include "render/handles.h"
#include "render/light.h"

namespace erwin
{

struct PassOptions
{
	enum DepthControl
	{
		DEPTH_CONTROL_AUTO = 0,
		DEPTH_CONTROL_FAR  = 1
	};

	static constexpr uint8_t  k_transparency_bits   = 1;
	static constexpr uint64_t k_transparency_shift  = uint64_t(64) - k_transparency_bits;
	static constexpr uint64_t k_transparency_mask   = uint64_t(1) << k_transparency_shift;
	static constexpr uint8_t  k_depth_control_bits  = 1;
	static constexpr uint64_t k_depth_control_shift = k_transparency_shift - k_depth_control_bits;
	static constexpr uint64_t k_depth_control_mask  = uint64_t(1) << k_depth_control_shift;
	static constexpr uint8_t  k_layer_id_bits       = 8;
	static constexpr uint64_t k_layer_id_shift      = k_depth_control_shift - k_layer_id_bits;
	static constexpr uint64_t k_layer_id_mask       = uint64_t(0xff) << k_layer_id_shift;

	inline void set_transparency(bool value)          { flags |= ((uint64_t(value) << k_transparency_shift)) & k_transparency_mask; }
	inline void set_depth_control(DepthControl value) { flags |= ((uint64_t(value) << k_transparency_shift)) & k_depth_control_mask; }
	inline void set_layer_id(uint8_t value)           { flags |= ((uint64_t(value) << k_layer_id_shift))     & k_layer_id_mask; }

	inline bool get_transparency() const              { return bool(         (flags & k_transparency_mask)  >> k_transparency_shift); }
	inline DepthControl get_depth_control() const     { return DepthControl( (flags & k_depth_control_bits) >> k_depth_control_shift); }
	inline uint8_t get_layer_id() const               { return uint8_t(      (flags & k_layer_id_mask)      >> k_layer_id_shift); }

	uint64_t flags = 0;
};

struct Material;
// 3D forward renderer front-end
class ForwardRenderer
{
public:
	// Register a shader for forward rendering
	static void register_shader(ShaderHandle shader, UniformBufferHandle material_ubo = UniformBufferHandle());
	// Start a new pass
	static void begin_pass(const PerspectiveCamera3D& camera, const DirectionalLight& dir_light, PassOptions options);
	// End a pass
	static void end_pass();
	// Draw a textured mesh
	// TMP: VertexArrayHandle argument will be replaced by a proper mesh handle
	static void draw_mesh(VertexArrayHandle VAO, const ComponentTransform3D& transform, const Material& material);

	// Stats
	static uint32_t get_draw_call_count();

private:
	friend class Application;

	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();
};

} // namespace erwin