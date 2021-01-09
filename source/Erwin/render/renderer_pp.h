#pragma once

#include "core/core.h"
#include "glm/glm.hpp"

namespace erwin
{

class EventBus;
// Post-processing renderer front-end
class PostProcessingRenderer
{
public:
	// Change final render target, default framebuffer initially
	static void set_final_render_target(hash_t framebuffer);
	// Execute bloom pass
	static void bloom_pass(hash_t framebuffer, uint32_t index);
	// Execute alternative implementation of bloom pass
	static void bloom_pass_alt(hash_t framebuffer, uint32_t index);
	// Apply post processing to an input framebuffer texture and blend it to the default framebuffer
	static void combine(hash_t framebuffer, uint32_t index, bool use_bloom);
	// Blend an input framebuffer texture to the default framebuffer using a "lighten" type blend function
	static void lighten(hash_t framebuffer, uint32_t index);

	// Post processing GUI
	static void on_imgui_render();

private:
	friend class Application;
	
	// Initialize renderer
	static void init(EventBus&);
	// Destroy renderer
	static void shutdown();
};

} // namespace erwin