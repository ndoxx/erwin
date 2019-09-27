#include "render/framebuffer_pool.h"
#include "core/intern_string.h"
#include "debug/logger.h"

namespace erwin
{


FramebufferPool::FramebufferPool(uint32_t initial_width, uint32_t initial_height):
current_width_(initial_width),
current_height_(initial_height)
{
	EVENTBUS.subscribe(this, &FramebufferPool::on_framebuffer_resize_event);
}

FramebufferPool::~FramebufferPool()
{
	
}

void FramebufferPool::create_framebuffer(hash_t name, WScope<FbConstraint> constraint, bool use_depth_texture)
{
	// Check that no framebuffer is already registered to this name
	auto it = framebuffers_.find(name);
	if(it != framebuffers_.end())
	{
		DLOGW("render") << "Framebuffer " << istr::resolve(name) << " already exists, ignoring." << std::endl;
		return;
	}

	uint32_t width  = constraint->get_width(current_width_);
	uint32_t height = constraint->get_width(current_height_);

	framebuffers_.insert(std::make_pair(name, Framebuffer::create(width, height, use_depth_texture)));
	constraints_.insert(std::make_pair(name, std::move(constraint)));
}

const Framebuffer& FramebufferPool::get_framebuffer(hash_t name) const
{
	auto it = framebuffers_.find(name);
	W_ASSERT(it != framebuffers_.end(), "No framebuffer by this name.");
	return *(it->second);
}

bool FramebufferPool::on_framebuffer_resize_event(const FramebufferResizeEvent& event)
{
	current_width_  = event.width;
	current_height_ = event.height;

	// Recreate each dynamic framebuffer to fit the new size, given its constraints
	for(auto&& [name, constraint]: constraints_)
	{
		// Fixed contraint ? Do nothing.
		if(constraint->is_fixed()) continue;

		uint32_t width  = constraint->get_width(current_width_);
		uint32_t height = constraint->get_width(current_height_);
		bool use_depth_texture = framebuffers_[name]->has_depth();

		framebuffers_[name] = Framebuffer::create(width, height, use_depth_texture);
	}

	return false;
}



} // namespace erwin