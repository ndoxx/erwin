#pragma once

#include <cstdlib>
#include <memory>

#include <kibble/hash/hash.h>
#include <kibble/assert/assert.h>

#ifdef _WIN32
	#ifdef W_BUILD_LIB
		#define W_API __declspec(dllexport)
	#else
		#define W_API __declspec(dllimport)
	#endif
#else
	#define W_API
#endif

[[maybe_unused]] static inline void fatal() { exit(-1); }

// Instrumentation timer
#ifdef W_PROFILE
	#include "debug/instrumentor.h"
	#define W_PROFILE_BEGIN_SESSION(name, filepath) Instrumentor::begin_session( name , filepath )
	#define W_PROFILE_END_SESSION(name) Instrumentor::end_session()
	#define W_PROFILE_ENABLE_SESSION(value) Instrumentor::set_session_enabled( value )
	#define W_PROFILE_SCOPE(name) InstrumentationTimer timer##__LINE__( name );
	#define W_PROFILE_FUNCTION() W_PROFILE_SCOPE( __PRETTY_FUNCTION__ )
#else
	#define W_PROFILE_BEGIN_SESSION(name, filepath)
	#define W_PROFILE_END_SESSION(name)
	#define W_PROFILE_ENABLE_SESSION(value)
	#define W_PROFILE_SCOPE(name)
	#define W_PROFILE_FUNCTION()
#endif

// Macro to disallow copy and assign
#define NON_COPYABLE( TYPE_NAME ) \
	TYPE_NAME(const TYPE_NAME&) = delete; \
	TYPE_NAME& operator=(const TYPE_NAME&) = delete

// Macro to make a class non-movable
#define NON_MOVABLE( TYPE_NAME ) \
	TYPE_NAME(TYPE_NAME&&) = delete; \
	TYPE_NAME& operator=(TYPE_NAME&&) = delete

namespace erwin
{
	// Ref counting pointer and unique pointer aliases (for now)
	template <class T>
	using WRef = std::shared_ptr<T>;

	template <class T>
	using WScope = std::unique_ptr<T>;

	// Factory methods for ref and scope as function alias, using perfect forwarding (for now)
	template <class T, class... Args>
	auto make_ref(Args&&... args) -> decltype(std::make_shared<T>(std::forward<Args>(args)...))
	{
	  return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <class T, class... Args>
	auto make_scope(Args&&... args) -> decltype(std::make_unique<T>(std::forward<Args>(args)...))
	{
	  return std::make_unique<T>(std::forward<Args>(args)...);
	}

	using hash_t = kb::hash_t;
} // namespace erwin