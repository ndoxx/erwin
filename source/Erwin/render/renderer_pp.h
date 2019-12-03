#pragma once

#include "core/wtypes.h"
#include "render/render_state.h" // For access to enums
#include "render/main_renderer.h"
#include "glm/glm.hpp"

namespace erwin
{

enum PPFlags: uint8_t
{
	PP_EN_CHROMATIC_ABERRATION = 1,
	PP_EN_EXPOSURE_TONE_MAPPING = 2,
	PP_EN_VIBRANCE = 4,
	PP_EN_SATURATION = 8,
	PP_EN_CONTRAST = 16,
	PP_EN_GAMMA = 32
};

// #pragma pack(push,1)
struct PostProcessingData
{
	void set_flag_enabled(PPFlags flag, bool value) { if(value) set_flag(flag); else clear_flag(flag); }
	void set_flag(PPFlags flag)   { flags |= flag; }
	void clear_flag(PPFlags flag) { flags &= ~flag; }
	bool get_flag(PPFlags flag)   { return (flags & flag); }

	glm::vec4 vib_balance = glm::vec4(0.5f); // Vibrance
	glm::vec4 cor_gamma = glm::vec4(1.f);    // Color correction
	float ca_shift = 0.f;                    // Chromatic aberration
	float ca_strength = 0.f;                 // Chromatic aberration
	float tm_exposure = 2.718f;              // Exposure tone mapping
	float vib_strength = 0.f;                // Vibrance
	float cor_saturation = 1.f;              // Color correction
	float cor_contrast = 1.f;                // Color correction
	
	// Filled in by renderer
	glm::vec2 fb_size;                       // Framebuffer size
	
	uint32_t flags = 0;						 // Flags to enable/disable post-processing features
};
// #pragma pack(pop)

// Post-processing renderer front-end
class PostProcessingRenderer
{
public:
	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();

	static void begin_pass(const PassState& state, const PostProcessingData& pp_data);
	static void blit(hash_t framebuffer, uint32_t index=0);
	static void end_pass();
};

} // namespace erwin