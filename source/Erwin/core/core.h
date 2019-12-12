#pragma once

#include <cstdlib>
#include <memory>

// Export JSON instrumentation profiles compatible with chrome://tracing
#define W_PROFILE
#define W_PROFILE_RENDER


#ifdef _WIN32
	#ifdef W_BUILD_LIB
		#define W_API __declspec(dllexport)
	#else
		#define W_API __declspec(dllimport)
	#endif
#else
	#define W_API
#endif

#ifdef W_ENABLE_ASSERT
	#include <cstdio>
	#define W_STATIC_ERROR(FSTR, ...) printf( FSTR , __VA_ARGS__ )
	#ifdef __linux__ 
		#include <csignal>
		#define W_ASSERT(CND, ...) { if(!( CND )) { printf("Assertion Failed: %s\n", __VA_ARGS__ ); raise(SIGTRAP); } }
	#endif
	#ifdef _WIN32 
		#define W_ASSERT(CND, ...) { if(!( CND )) { printf("Assertion Failed: %s\n", __VA_ARGS__ ); __debugbreak(); } }
	#endif
#else
	#define W_STATIC_ERROR(FSTR, ...)
	#define W_ASSERT(CND, ...)
#endif

inline void fatal() { exit(-1); }

// Instrumentation timer
#ifdef W_PROFILE
	#include "debug/instrumentor.h"
	#define W_PROFILE_BEGIN_SESSION(name, filepath) erwin::Instrumentor::begin_session( name , filepath )
	#define W_PROFILE_END_SESSION(name) erwin::Instrumentor::end_session()
	#define W_PROFILE_ENABLE_SESSION(value) erwin::Instrumentor::set_session_enabled( value )
	#define W_PROFILE_SCOPE(name) erwin::InstrumentationTimer timer##__LINE__( name );
	#define W_PROFILE_FUNCTION() W_PROFILE_SCOPE( __PRETTY_FUNCTION__ )
#else
	#define W_PROFILE_BEGIN_SESSION(name, filepath)
	#define W_PROFILE_END_SESSION(name)
	#define W_PROFILE_ENABLE_SESSION(value)
	#define W_PROFILE_SCOPE(name)
	#define W_PROFILE_FUNCTION()
#endif
#ifdef W_PROFILE_RENDER
	#define W_PROFILE_RENDER_SCOPE(name) erwin::InstrumentationTimer timer##__LINE__( name );
	#define W_PROFILE_RENDER_FUNCTION() W_PROFILE_RENDER_SCOPE( __PRETTY_FUNCTION__ )
#else
	#define W_PROFILE_RENDER_SCOPE(name)
	#define W_PROFILE_RENDER_FUNCTION()
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
} // namespace erwin