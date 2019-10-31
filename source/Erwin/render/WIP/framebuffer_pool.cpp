#include "render/WIP/framebuffer_pool.h"
#include "render/WIP/main_renderer.h"
#include "core/intern_string.h"
#include "debug/logger.h"

namespace erwin
{
namespace WIP
{

struct FramebufferPoolStorage
{
	std::map<hash_t, FramebufferHandle> framebuffers_;
	std::map<hash_t, WScope<FbConstraint>> constraints_;

	uint32_t current_width_;
	uint32_t current_height_;
};
static FramebufferPoolStorage s_storage;

static bool on_framebuffer_resize_event(const FramebufferResizeEvent& event)
{
	s_storage.current_width_  = event.width;
	s_storage.current_height_ = event.height;

	auto& rq = MainRenderer::get_queue(MainRenderer::Resource);
	// Recreate each dynamic framebuffer to fit the new size, given its constraints
	for(auto&& [name, constraint]: s_storage.constraints_)
	{
		// Fixed contraint ? Do nothing.
		if(constraint->is_fixed()) continue;

		uint32_t width  = constraint->get_width(s_storage.current_width_);
		uint32_t height = constraint->get_width(s_storage.current_height_);

		rq.update_framebuffer(s_storage.framebuffers_[name], width, height);
	}

	return false;
}

void FramebufferPool::init(uint32_t initial_width, uint32_t initial_height)
{
	s_storage.current_width_  = initial_width;
	s_storage.current_height_ = initial_height;

	EVENTBUS.subscribe(&on_framebuffer_resize_event);
	DLOGN("render") << "Framebuffer pool created." << std::endl;
}

void FramebufferPool::shutdown()
{
	auto& rq = MainRenderer::get_queue(MainRenderer::Resource);
	for(auto&& [name, fb]: s_storage.framebuffers_)
		rq.destroy_framebuffer(fb);

	DLOGN("render") << "Framebuffer pool released." << std::endl;
}

FramebufferHandle FramebufferPool::create_framebuffer(hash_t name, WScope<FbConstraint> constraint, const FramebufferLayout& layout, bool depth, bool stencil)
{
	// Check that no framebuffer is already registered to this name
	auto it = s_storage.framebuffers_.find(name);
	if(it != s_storage.framebuffers_.end())
	{
		DLOGW("render") << "Framebuffer " << istr::resolve(name) << " already exists, ignoring." << std::endl;
		return FramebufferHandle();
	}

	uint32_t width  = constraint->get_width(s_storage.current_width_);
	uint32_t height = constraint->get_width(s_storage.current_height_);

	auto& rq = MainRenderer::get_queue(MainRenderer::Resource);
	FramebufferHandle handle = rq.create_framebuffer(width, height, depth, stencil, layout);
	s_storage.framebuffers_.insert(std::make_pair(name, handle));
	s_storage.constraints_.insert(std::make_pair(name, std::move(constraint)));

	return handle;
}

} // namespace WIP
} // namespace erwin