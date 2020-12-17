#include "render/framebuffer_pool.h"
#include "render/renderer.h"
#include "core/intern_string.h"
#include <kibble/logger/logger.h>
#include "event/event_bus.h"
#include "event/window_events.h"

namespace erwin
{

struct FramebufferPoolStorage
{
	std::map<hash_t, FramebufferHandle> framebuffers_;
	std::map<hash_t, WScope<FbConstraint>> constraints_;
	std::map<hash_t, uint8_t> flags_;

	uint32_t current_width_;
	uint32_t current_height_;
};
static FramebufferPoolStorage s_storage;

static bool on_framebuffer_resize_event(const FramebufferResizeEvent& event)
{
	s_storage.current_width_  = uint32_t(event.width);
	s_storage.current_height_ = uint32_t(event.height);

	// Recreate each dynamic framebuffer to fit the new size, given its constraints
	for(auto&& [name, constraint]: s_storage.constraints_)
	{
		// Fixed contraint ? Do nothing.
		if(constraint->is_fixed()) continue;

		uint32_t width  = constraint->get_width(s_storage.current_width_);
		uint32_t height = constraint->get_width(s_storage.current_height_);

		Renderer::update_framebuffer(s_storage.framebuffers_[name], width, height);
	}

	return false;
}

void FramebufferPool::init(uint32_t initial_width, uint32_t initial_height)
{
	s_storage.current_width_  = initial_width;
	s_storage.current_height_ = initial_height;

	EventBus::subscribe(&on_framebuffer_resize_event);
	KLOGN("render") << "Framebuffer pool created." << std::endl;
}

void FramebufferPool::shutdown()
{
	for(auto&& [name, fb]: s_storage.framebuffers_)
		Renderer::destroy(fb);

	KLOGN("render") << "Framebuffer pool released." << std::endl;
}

FramebufferHandle FramebufferPool::get_framebuffer(hash_t name)
{
	// Check that a framebuffer is registered to this name
	auto it = s_storage.framebuffers_.find(name);
	K_ASSERT(it != s_storage.framebuffers_.end(), "[FramebufferPool] Invalid framebuffer name.");
	return it->second;
}

void FramebufferPool::traverse_framebuffers(std::function<void(FramebufferHandle)> visitor)
{
	for(auto&& [key, handle]: s_storage.framebuffers_)
		visitor(handle);
}

bool FramebufferPool::has_depth(hash_t name)
{
	auto it = s_storage.flags_.find(name);
	K_ASSERT(it != s_storage.flags_.end(), "[FramebufferPool] Invalid framebuffer name.");
	return bool(it->second & FBFlag::FB_DEPTH_ATTACHMENT);
}

uint32_t FramebufferPool::get_width(hash_t name)
{
	auto it = s_storage.constraints_.find(name);
	K_ASSERT(it != s_storage.constraints_.end(), "[FramebufferPool] Invalid framebuffer name.");
	return it->second->get_width(s_storage.current_width_);
}

uint32_t FramebufferPool::get_height(hash_t name)
{
	auto it = s_storage.constraints_.find(name);
	K_ASSERT(it != s_storage.constraints_.end(), "[FramebufferPool] Invalid framebuffer name.");
	return it->second->get_height(s_storage.current_height_);
}

glm::vec2 FramebufferPool::get_size(hash_t name)
{
	auto it = s_storage.constraints_.find(name);
	K_ASSERT(it != s_storage.constraints_.end(), "[FramebufferPool] Invalid framebuffer name.");
	return {it->second->get_width(s_storage.current_width_), it->second->get_height(s_storage.current_height_)};
}

glm::vec2 FramebufferPool::get_texel_size(hash_t name)
{
	auto it = s_storage.constraints_.find(name);
	K_ASSERT(it != s_storage.constraints_.end(), "[FramebufferPool] Invalid framebuffer name.");
	return {1.f/float(it->second->get_width(s_storage.current_width_)), 1.f/float(it->second->get_height(s_storage.current_height_))};
}

uint32_t FramebufferPool::get_screen_width()
{
	return s_storage.current_width_;
}

uint32_t FramebufferPool::get_screen_height()
{
	return s_storage.current_height_;
}

glm::vec2 FramebufferPool::get_screen_size()
{
	return {s_storage.current_width_, s_storage.current_height_};
}

glm::vec2 FramebufferPool::get_screen_texel_size()
{
	return {1.f/float(s_storage.current_width_), 1.f/float(s_storage.current_height_)};
}

FramebufferHandle FramebufferPool::create_framebuffer(hash_t name, WScope<FbConstraint> constraint, uint8_t flags, const FramebufferLayout& layout)
{
	// Check that no framebuffer is already registered to this name
	auto it = s_storage.framebuffers_.find(name);
	if(it != s_storage.framebuffers_.end())
	{
		KLOGW("render") << "Framebuffer " << istr::resolve(name) << " already exists, ignoring." << std::endl;
		return FramebufferHandle();
	}

	uint32_t width  = constraint->get_width(s_storage.current_width_);
	uint32_t height = constraint->get_width(s_storage.current_height_);

	FramebufferHandle handle = Renderer::create_framebuffer(width, height, flags, layout);
	s_storage.framebuffers_.insert(std::make_pair(name, handle));
	s_storage.constraints_.insert(std::make_pair(name, std::move(constraint)));
	s_storage.flags_.insert(std::make_pair(name, flags));

	KLOG("render",1) << "Submit framebuffer creation: " << kb::KS_NAME_ << istr::resolve(name) << ' ' << kb::KC_ << handle << std::endl;

	return handle;
}

} // namespace erwin