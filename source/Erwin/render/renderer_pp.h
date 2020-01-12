#pragma once

#include "core/wtypes.h"
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
	PP_EN_GAMMA = 32,
	PP_EN_FXAA = 64,
	PP_EN_BLOOM = 128
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
	// Execute bloom pass
	static void bloom_pass(hash_t framebuffer, uint32_t index);
	// Execute alternative implementation of bloom pass
	static void bloom_pass_alt(hash_t framebuffer, uint32_t index);
	// Apply post processing to an input framebuffer texture and blend it to the default framebuffer
	static void combine(hash_t framebuffer, uint32_t index, const PostProcessingData& pp_data);
	// Blend an input framebuffer texture to the default framebuffer using a "lighten" type blend function
	static void lighten(hash_t framebuffer, uint32_t index);

	// TMP: reset sequence number used by combine() and lighten() for the computation of command sorting keys
	static void reset_sequence();

private:
	friend class Application;
	
	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();
};

} // namespace erwin