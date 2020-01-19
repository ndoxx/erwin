#include <map>

#include "render/renderer_2d.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "asset/asset_manager.h"
#include "asset/texture_atlas.h"
#include "core/config.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/vec_swizzle.hpp"

namespace erwin
{
	
struct InstanceData
{

};

struct Batch2D
{

};

static struct
{

} s_storage;

// TMP: MOVE this to proper collision trait class?
static bool frustum_cull(const glm::vec2& position, const glm::vec2& scale, const FrustumSides& fs)
{
	// Compute each point in world space
	glm::vec3 points[4] =
	{
		// WTF: Scale terms should be multiplied by 0.5f to make sense,
		// but it produces false positives at the edges.
		glm::vec3(position.x-scale.x, position.y-scale.y, 1.0f),
		glm::vec3(position.x+scale.x, position.y-scale.y, 1.0f),
		glm::vec3(position.x+scale.x, position.y+scale.y, 1.0f),
		glm::vec3(position.x-scale.x, position.y+scale.y, 1.0f)
	};

    // For each frustum side
    for(uint32_t ii=0; ii<4; ++ii)
    {
        // Quad is considered outside iif all its vertices are above the SAME side
        bool all_out = true;
        for(const auto& p: points)
        {
            // Check if point is above side
            if(glm::dot(fs.side[ii],p)>0)
            {
                all_out = false;
                break;
            }
        }
        if(all_out)
            return true;
    }

	return false;
}

static void create_batch(uint16_t index, TextureHandle handle)
{

}

void Renderer2D::init()
{
    W_PROFILE_FUNCTION()

    FramebufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    FramebufferPool::create_framebuffer("SpriteBuffer"_h, make_scope<FbRatioConstraint>(), layout, true);


}

void Renderer2D::shutdown()
{
    W_PROFILE_FUNCTION()


}

void Renderer2D::create_batch(TextureHandle handle)
{
	::erwin::create_batch(handle.index, handle);
}

void Renderer2D::begin_pass(const OrthographicCamera2D& camera, bool transparent)
{
    W_PROFILE_FUNCTION()


}

void Renderer2D::end_pass()
{
    W_PROFILE_FUNCTION()
    
	Renderer2D::flush();
}

static void flush_batch(Batch2D& batch)
{

}

void Renderer2D::draw_quad(const ComponentTransform2D& transform, TextureAtlasHandle atlas_handle, hash_t tile, const glm::vec4& tint)
{

}

void Renderer2D::draw_colored_quad(const ComponentTransform2D& transform, const glm::vec4& tint)
{

}

void Renderer2D::draw_text(const std::string& text, FontAtlasHandle font_handle, float x, float y, float scale, const glm::vec4& tint)
{

}


void Renderer2D::flush()
{

}

uint32_t Renderer2D::get_draw_call_count()
{

}

} // namespace erwin