#include "render/framebuffer_pool.h"
#include "render/render_device.h"
#include "core/intern_string.h"
#include "debug/logger.h"

namespace erwin
{


FramebufferPool::FramebufferPool(uint32_t initial_width, uint32_t initial_height):
current_width_(initial_width),
current_height_(initial_height)
{
	EVENTBUS.subscribe(this, &FramebufferPool::on_framebuffer_resize_event);
	DLOG("render",1) << "Framebuffer pool created." << std::endl;
}

FramebufferPool::~FramebufferPool()
{
	
}

void FramebufferPool::create_framebuffer(hash_t name, WScope<FbConstraint> constraint, const FramebufferLayout& layout, bool depth, bool stencil)
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

	framebuffers_.insert(std::make_pair(name, Framebuffer::create(width, height, layout, depth, stencil)));
	constraints_.insert(std::make_pair(name, std::move(constraint)));
}

const Framebuffer& FramebufferPool::get(hash_t name) const
{
	auto it = framebuffers_.find(name);
	W_ASSERT(it != framebuffers_.end(), "No framebuffer by this name.");
	return *(it->second);
}

void FramebufferPool::bind(hash_t name) const
{
	if(name==0)
	{
		Gfx::device->bind_default_frame_buffer();
		return;
	}
	auto it = framebuffers_.find(name);
	W_ASSERT(it != framebuffers_.end(), "No framebuffer by this name.");
	it->second->bind();
}

bool FramebufferPool::exists(hash_t name) const
{
	return (framebuffers_.find(name) != framebuffers_.end());
}

const Texture2D& FramebufferPool::get_texture(hash_t name, uint32_t index)
{
	auto it = framebuffers_.find(name);
	W_ASSERT(it != framebuffers_.end(), "No framebuffer by this name.");
	return it->second->get_texture(index);
}

const Texture2D& FramebufferPool::get_named_texture(hash_t fbname, hash_t texname)
{
	auto it = framebuffers_.find(fbname);
	W_ASSERT(it != framebuffers_.end(), "No framebuffer by this name.");
	return it->second->get_named_texture(texname);
}

void FramebufferPool::release()
{
	framebuffers_.clear();
	DLOG("render",1) << "Framebuffer pool released." << std::endl;
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
		bool has_depth = framebuffers_[name]->has_depth();
		bool has_stencil = framebuffers_[name]->has_stencil();
		auto layout = framebuffers_[name]->get_layout();

		framebuffers_[name] = Framebuffer::create(width, height, layout, has_depth, has_stencil);
	}

	return false;
}



} // namespace erwin